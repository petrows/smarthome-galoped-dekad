// zigbee2mqtt external converter for the Galoped gauge device.
//
// Install: copy this file to your z2m data directory (next to configuration.yaml)
// and add to configuration.yaml:
//
//   external_converters:
//     - galoped-converter.js
//
// The firmware advertises:
//   EP 10 — Color Dimmable Light  (On/Off, Level, Color XY + HueSat)
//   EP 20 — Drive 1 (Analog Output + custom cluster 0xFC10)
//   EP 21 — Drive 2 (Analog Output + custom cluster 0xFC10)
//   EP 30 — Climate sensor (Temperature/Humidity/Pressure, only on GALOPED_CLIMATE=1 builds)
//   EP 40 — SenseAir S8 CO2 sensor (only on GALOPED_CO2=1 builds)
//
// Custom cluster 0xFC10 attributes:
//   0x0000 position   (int32, RW, reportable) — absolute step count
//   0x0001 max_steps  (uint16, RO)            — driver max-steps cap
//   0x0002 is_moving  (bool, RO, reportable)
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
        if (msg.data.isMoving !== undefined) result['is_moving' + s] = msg.data.isMoving !== 0;
        if (msg.data.maxSteps !== undefined) result['max_steps' + s] = msg.data.maxSteps;
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
    e.binary('is_moving', ea.STATE, true, false)
        .withDescription('Drive is currently moving')
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

    endpoint: (device) => ({
        light: 10,
        drive_1: 20,
        drive_2: 21,
        climate: 30,
        co2: 40,
    }),

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

    exposes: [
        e.light()
            .withBrightness()
            .withColor(['hs', 'xy'])
            .withEndpoint('light'),
        // maxSteps in the converter only constrains the UI slider — the firmware
        // also clamps server-side. Adjust if you change Config::maxSteps.
        ...driveExposes('drive_1', 3950),
        ...driveExposes('drive_2', 3295),
        // Climate endpoint — exposed only on firmware built with GALOPED_CLIMATE=1.
        // If absent on a given board, the device simply never reports these and
        // z2m will show them as unavailable (no harm to non-climate variants).
        e.temperature().withEndpoint('climate'),
        e.humidity().withEndpoint('climate'),
        e.pressure().withEndpoint('climate'),
        // CO2 endpoint — only on GALOPED_CO2=1 builds.
        e.co2().withEndpoint('co2'),
    ],

    configure: async (device, coordinatorEndpoint, logger) => {
        addDriveCluster(device);

        const lightEp = device.getEndpoint(10);
        await reporting.bind(lightEp, coordinatorEndpoint,
            ['genOnOff', 'genLevelCtrl', 'lightingColorCtrl']);
        await reporting.onOff(lightEp);
        await reporting.brightness(lightEp);

        for (const epId of [20, 21]) {
            const ep = device.getEndpoint(epId);
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

            // Report IsMoving on any change, 0..300s
            await ep.configureReporting(CLUSTER, [{
                attribute: 'isMoving',
                minimumReportInterval: 0,
                maximumReportInterval: 300,
                reportableChange: 0,
            }]);

            // One-shot read so MaxSteps lands in state
            await ep.read(CLUSTER, ['maxSteps']);
        }

        // Climate endpoint — present only on GALOPED_CLIMATE=1 builds.
        // Wrap in try/catch so a non-climate variant doesn't fail interview.
        const climateEp = device.getEndpoint(30);
        if (climateEp) {
            try {
                await reporting.bind(climateEp, coordinatorEndpoint,
                    ['msTemperatureMeasurement', 'msRelativeHumidity', 'msPressureMeasurement']);
                // Temperature: every 30..300 s, on >= 0.1 °C change
                await reporting.temperature(climateEp,
                    {min: 30, max: 300, change: 10});
                // Humidity: every 30..300 s, on >= 1 %RH change
                await reporting.humidity(climateEp,
                    {min: 30, max: 300, change: 100});
                // Pressure: every 60..600 s, on >= 1 hPa change
                await reporting.pressure(climateEp,
                    {min: 60, max: 600, change: 1});
            } catch (err) {
                logger.warn(`Climate EP 30 binding failed (firmware without GALOPED_CLIMATE?): ${err.message}`);
            }
        }

        // CO2 endpoint — present only on GALOPED_CO2=1 builds.
        const co2Ep = device.getEndpoint(40);
        if (co2Ep) {
            try {
                await reporting.bind(co2Ep, coordinatorEndpoint, ['msCO2']);
                // measuredValue is a fraction of 1 (1000 ppm = 0.001).
                // Report every 30..600 s on >= 50 ppm change.
                await co2Ep.configureReporting('msCO2', [{
                    attribute: 'measuredValue',
                    minimumReportInterval: 30,
                    maximumReportInterval: 600,
                    reportableChange: 0.00005,
                }]);
            } catch (err) {
                logger.warn(`CO2 EP 40 binding failed (firmware without GALOPED_CO2?): ${err.message}`);
            }
        }
    },
};

module.exports = definition;
