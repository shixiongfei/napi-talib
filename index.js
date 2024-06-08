/*
 * index.js
 *
 * Copyright (c) 2024 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

const types = require("./types.js");

try {
  const binding = require("./build/Release/napi_talib.node");
  module.exports = Object.assign(binding, types);
} catch {
  const binding = require("./build/Debug/napi_talib.node");
  module.exports = Object.assign(binding, types);
}
