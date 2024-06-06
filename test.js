/*
 * test.js
 *
 * Copyright (c) 2024 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

const fs = require("node:fs");
const talib = require(".");

const marketData = JSON.parse(fs.readFileSync("./marketdata.json", "utf8"));

console.log(talib.version());

console.log(talib.getFunctionGroups());

console.log(talib.getFunctions());

console.log(talib.explain("ADX"));

console.log(
  talib.execute({
    name: "ADX",
    startIdx: 0,
    endIdx: marketData.close.length - 1,
    high: marketData.high,
    low: marketData.low,
    close: marketData.close,
    optInTimePeriod: 9,
  })
);

console.log(
  talib.execute({}, (error, result) => {
    if (error) {
      console.error(error instanceof Error ? error.message : error);
    } else {
      console.log(result);
    }
  })
);
