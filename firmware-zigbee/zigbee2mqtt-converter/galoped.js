// zigbee2mqtt external converter for the Galoped gauge device.
//
// Install: copy this file to your z2m data directory (next to configuration.yaml)
// and add to configuration.yaml:
//
//   external_converters:
//     - galoped-converter.js
//
// The firmware advertises:
//   EP 10 — Color Dimmable Light  (On/Off, Level, Color XY)
//   EP 20 — Drive 1 (Analog Output + custom cluster 0xFC10)
//   EP 21 — Drive 2 (only on GALOPED_AXIS>1 builds)
//   EP 30 — Climate sensor (Temperature/Humidity/Pressure, only on GALOPED_CLIMATE=1)
//   EP 40 — SenseAir S8 CO2 sensor (only on GALOPED_CO2=1)
//
// The converter detects model variant by which endpoints are actually present,
// so a single converter file covers moax / biax / co2 firmware builds.
//
// Custom cluster 0xFC10 attributes:
//   0x0000 position   (int32, RW, reportable) — absolute step count
//   0x0001 max_steps  (uint16, RO)            — driver max-steps cap
// Custom command 0x00 reset                   — runs the blocking zero() routine

const {Zcl} = require('zigbee-herdsman');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');

const e = exposes.presets;
const ea = exposes.access;

const CLUSTER = 'manuSpecificGalopedDrive';
const CLUSTER_ID = 0xfc10;

const EP_LIGHT = 10;
const EP_DRIVE_1 = 20;
const EP_DRIVE_2 = 21;
const EP_CLIMATE = 30;
const EP_CO2 = 40;

// True if the device's interview captured an endpoint with this ID. When
// `device` is a placeholder (e.g. during definition discovery) we return true
// so every expose is shown — the runtime will gate again after the interview.
function hasEp(device, id) {
    if (!device || !Array.isArray(device.endpoints)) return true;
    return device.endpoints.some((ep) => ep.ID === id);
}

// z-h-c's built-in fz.pressure / fz.co2 (this version) don't apply the
// multiEndpoint suffix, so reports come out as bare 'pressure' / 'co2'.
// This helper mirrors what utils.postfixWithEndpointName does for fz.temperature.
function endpointSuffix(msg, model) {
    if (!model.meta || !model.meta.multiEndpoint) return '';
    const map = model.endpoint(msg.device);
    for (const name in map) {
        if (map[name] === msg.endpoint.ID) return '_' + name;
    }
    return '';
}

function addDriveCluster(device) {
    if (device.customClusters && device.customClusters[CLUSTER]) {
        return;
    }
    device.addCustomCluster(CLUSTER, {
        ID: CLUSTER_ID,
        attributes: {
            position: {ID: 0x0000, type: Zcl.DataType.INT32},
            maxSteps: {ID: 0x0001, type: Zcl.DataType.UINT16},
            isMoving: {ID: 0x0002, type: Zcl.DataType.BOOLEAN},
        },
        commands: {
            reset: {ID: 0x00, parameters: []},
        },
        commandsResponse: {},
    });
}

// z2m only adds the endpoint suffix to tx (outgoing /set) keys and tz lookups.
// Incoming fz must return already-suffixed keys, otherwise state ends up with
// bare `position` / `is_moving` etc. that collide between endpoints.

const fzDrive = {
    cluster: CLUSTER,
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const s = endpointSuffix(msg, model);
        const result = {};
        if (msg.data.position !== undefined) result['position' + s] = msg.data.position;
        if (msg.data.maxSteps !== undefined) {
            result['max_steps' + s] = msg.data.maxSteps;
            // exposes() reads this value to set the position slider's max — ask z2m
            // to re-emit exposes so the UI picks up the new bound.
            meta.deviceExposesChanged?.();
        }
        return result;
    },
};

const fzAnalogOutput = {
    cluster: 'genAnalogOutput',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        if (msg.data.presentValue === undefined) return;
        return {['position' + endpointSuffix(msg, model)]: Math.round(msg.data.presentValue)};
    },
};

const fzPressure = {
    cluster: 'msPressureMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        if (msg.data.measuredValue === undefined) return;
        return {['pressure' + endpointSuffix(msg, model)]: msg.data.measuredValue};
    },
};

const fzCO2 = {
    cluster: 'msCO2',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        if (msg.data.measuredValue === undefined) return;
        // ZCL stores CO2 as fraction of 1; convert back to ppm.
        return {['co2' + endpointSuffix(msg, model)]: Math.round(msg.data.measuredValue * 1000000)};
    },
};

const tzPosition = {
    key: ['position'],
    convertSet: async (entity, key, value, meta) => {
        const steps = Number(value) | 0;
        // Write the standard Analog Output PresentValue. The firmware mirrors
        // it onto the custom cluster, so both stay in sync.
        await entity.write('genAnalogOutput', {presentValue: steps});
        return {state: {position: steps}};
    },
    convertGet: async (entity, key, meta) => {
        await entity.read('genAnalogOutput', ['presentValue']);
    },
};

const tzReset = {
    key: ['reset'],
    convertSet: async (entity, key, value, meta) => {
        await entity.command(CLUSTER, 'reset', {});
        return {};
    },
};

const driveExposes = (endpointName, maxSteps) => [
    e.numeric('position', ea.ALL)
        .withValueMin(0)
        .withValueMax(maxSteps)
        .withUnit('steps')
        .withDescription('Drive absolute position')
        .withEndpoint(endpointName),
    e.numeric('max_steps', ea.STATE)
        .withDescription('Drive max steps (from firmware)')
        .withEndpoint(endpointName),
    e.enum('reset', ea.SET, ['reset'])
        .withDescription('Run blocking zero/calibration sweep')
        .withEndpoint(endpointName),
];

const definition = {
    zigbeeModel: ['galoped'],
    model: 'galoped',
    vendor: 'Galoped',
    description: 'Galoped Zigbee gauge — RGB indicator + 2 stepper drives',

    endpoint: (device) => {
        const map = {light: EP_LIGHT};
        if (hasEp(device, EP_DRIVE_1)) map.drive_1 = EP_DRIVE_1;
        if (hasEp(device, EP_DRIVE_2)) map.drive_2 = EP_DRIVE_2;
        if (hasEp(device, EP_CLIMATE)) map.climate = EP_CLIMATE;
        if (hasEp(device, EP_CO2)) map.co2 = EP_CO2;
        return map;
    },

    meta: {multiEndpoint: true},

    onEvent: async (type, data, device) => {
        addDriveCluster(device);
    },

    fromZigbee: [
        fz.on_off, fz.brightness, fz.color_colortemp,
        fzAnalogOutput, fzDrive,
        fz.temperature, fz.humidity, fzPressure,
        fzCO2,
    ],

    toZigbee: [
        tz.on_off, tz.light_onoff_brightness, tz.light_color,
        tzPosition, tzReset,
    ],

    // Exposes computed dynamically from the device's actual endpoint set:
    // drive 2, climate, and CO2 only appear when their EPs were captured in
    // the interview. Position sliders honor each drive's maxSteps as read
    // from the custom cluster (0xFC10/0x0001) so the converter never needs
    // recompiling when CONFIG_GALOPED_DRIVE_*_MAX_STEPS changes.
    exposes: (device, options) => {
        // Bridge calls exposes() at startup before onEvent has fired, so the
        // custom cluster name isn't resolvable yet — register it here too.
        if (device && typeof device.addCustomCluster === 'function') {
            addDriveCluster(device);
        }
        const driveMax = (epId) => {
            try {
                const ep = device && typeof device.getEndpoint === 'function'
                    ? device.getEndpoint(epId) : undefined;
                const v = ep && ep.getClusterAttributeValue
                    ? ep.getClusterAttributeValue(CLUSTER, 'maxSteps') : undefined;
                return typeof v === 'number' && v > 0 ? v : 65535;
            } catch (_e) {
                return 65535;
            }
        };
        const result = [
            e.light()
                .withBrightness()
                .withColor(['xy'])
                .withEndpoint('light'),
        ];
        if (hasEp(device, EP_DRIVE_1)) result.push(...driveExposes('drive_1', driveMax(EP_DRIVE_1)));
        if (hasEp(device, EP_DRIVE_2)) result.push(...driveExposes('drive_2', driveMax(EP_DRIVE_2)));
        if (hasEp(device, EP_CLIMATE)) {
            result.push(
                e.temperature().withEndpoint('climate'),
                e.humidity().withEndpoint('climate'),
                e.pressure().withEndpoint('climate'),
            );
        }
        if (hasEp(device, EP_CO2)) {
            result.push(e.co2().withEndpoint('co2'));
        }
        return result;
    },

    configure: async (device, coordinatorEndpoint, logger) => {
        addDriveCluster(device);

        const lightEp = device.getEndpoint(EP_LIGHT);
        if (lightEp) {
            await reporting.bind(lightEp, coordinatorEndpoint,
                ['genOnOff', 'genLevelCtrl', 'lightingColorCtrl']);
            await reporting.onOff(lightEp);
            await reporting.brightness(lightEp);
        }

        for (const epId of [EP_DRIVE_1, EP_DRIVE_2]) {
            const ep = device.getEndpoint(epId);
            if (!ep) continue;  // drive 2 absent on single-axis builds
            await reporting.bind(ep, coordinatorEndpoint, ['genAnalogOutput', CLUSTER]);

            // Report PresentValue on change (>=1 step) every 1..60s
            await ep.configureReporting('genAnalogOutput', [{
                attribute: 'presentValue',
                minimumReportInterval: 1,
                maximumReportInterval: 60,
                reportableChange: 1,
            }]);

            // Report Position on change (>=1 step), 1..60s
            await ep.configureReporting(CLUSTER, [{
                attribute: 'position',
                minimumReportInterval: 1,
                maximumReportInterval: 60,
                reportableChange: 1,
            }]);

            // One-shot read so MaxSteps lands in state
            await ep.read(CLUSTER, ['maxSteps']);
        }

        const climateEp = device.getEndpoint(EP_CLIMATE);
        if (climateEp) {
            await reporting.bind(climateEp, coordinatorEndpoint,
                ['msTemperatureMeasurement', 'msRelativeHumidity', 'msPressureMeasurement']);
            await reporting.temperature(climateEp, {min: 30, max: 300, change: 10});
            await reporting.humidity(climateEp, {min: 30, max: 300, change: 100});
            await reporting.pressure(climateEp, {min: 60, max: 600, change: 1});
        }

        const co2Ep = device.getEndpoint(EP_CO2);
        if (co2Ep) {
            await reporting.bind(co2Ep, coordinatorEndpoint, ['msCO2']);
            // measuredValue is a fraction of 1 (1000 ppm = 0.001).
            // Report every 30..600 s on >= 50 ppm change.
            await co2Ep.configureReporting('msCO2', [{
                attribute: 'measuredValue',
                minimumReportInterval: 30,
                maximumReportInterval: 600,
                reportableChange: 0.00005,
            }]);
        }
    },
};

module.exports = definition;
