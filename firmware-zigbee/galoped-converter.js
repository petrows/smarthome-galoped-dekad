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

const fzDrive = {
    cluster: CLUSTER,
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const suffix = msg.endpoint.ID === 20 ? '_drive_1' : '_drive_2';
        const result = {};
        if (msg.data.position !== undefined) {
            result['position' + suffix] = msg.data.position;
        }
        if (msg.data.isMoving !== undefined) {
            result['is_moving' + suffix] = msg.data.isMoving !== 0;
        }
        if (msg.data.maxSteps !== undefined) {
            result['max_steps' + suffix] = msg.data.maxSteps;
        }
        return result;
    },
};

const fzAnalogOutput = {
    cluster: 'genAnalogOutput',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        if (msg.data.presentValue === undefined) return;
        const suffix = msg.endpoint.ID === 20 ? '_drive_1' : '_drive_2';
        return {['position' + suffix]: Math.round(msg.data.presentValue)};
    },
};

const tzPosition = {
    key: ['position_drive_1', 'position_drive_2'],
    convertSet: async (entity, key, value, meta) => {
        const endpointId = key === 'position_drive_1' ? 20 : 21;
        const endpoint = meta.device.getEndpoint(endpointId);
        const steps = Number(value) | 0;
        // Write the standard Analog Output PresentValue. The firmware mirrors
        // it onto the custom cluster, so both stay in sync.
        await endpoint.write('genAnalogOutput', {presentValue: steps});
        return {state: {[key]: steps}};
    },
    convertGet: async (entity, key, meta) => {
        const endpointId = key === 'position_drive_1' ? 20 : 21;
        const endpoint = meta.device.getEndpoint(endpointId);
        await endpoint.read('genAnalogOutput', ['presentValue']);
    },
};

const tzReset = {
    key: ['reset_drive_1', 'reset_drive_2'],
    convertSet: async (entity, key, value, meta) => {
        const endpointId = key === 'reset_drive_1' ? 20 : 21;
        const endpoint = meta.device.getEndpoint(endpointId);
        await endpoint.command(CLUSTER, 'reset', {});
        return {};
    },
};

const driveExposes = (suffix, maxSteps) => [
    e.numeric('position' + suffix, ea.ALL)
        .withValueMin(0)
        .withValueMax(maxSteps)
        .withUnit('steps')
        .withDescription('Drive absolute position'),
    e.binary('is_moving' + suffix, ea.STATE, true, false)
        .withDescription('Drive is currently moving'),
    e.numeric('max_steps' + suffix, ea.STATE)
        .withDescription('Drive max steps (from firmware)'),
    e.enum('reset' + suffix, ea.SET, ['reset'])
        .withDescription('Run blocking zero/calibration sweep'),
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
    }),

    meta: {multiEndpoint: true},

    onEvent: async (type, data, device) => {
        addDriveCluster(device);
    },

    fromZigbee: [
        fz.on_off, fz.brightness, fz.color_colortemp,
        fzAnalogOutput, fzDrive,
    ],

    toZigbee: [
        tz.on_off, tz.light_brightness, tz.light_color,
        tzPosition, tzReset,
    ],

    exposes: [
        e.light()
            .withBrightness()
            .withColorHS()
            .withColorXY()
            .withEndpoint('light'),
        // maxSteps in the converter only constrains the UI slider — the firmware
        // also clamps server-side. Adjust if you change Config::maxSteps.
        ...driveExposes('_drive_1', 3950),
        ...driveExposes('_drive_2', 3295),
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
    },
};

module.exports = definition;
