'use strict';

const heatshrink_node = require('bindings')('heatshrink_node');

exports.encode = heatshrink_node.encode;