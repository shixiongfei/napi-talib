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

console.log(talib.MAType.EMA);

console.log(talib.getFunctionGroups());

console.log(talib.getFunctions());

console.log(talib.setUnstablePeriod(talib.FuncUnstId.EMA, 14));

console.log(talib.explain("ADX"));

console.log(
  talib.execute({
    name: "ADX",
    startIdx: 0,
    endIdx: marketData.close.length - 1,
    params: {
      high: marketData.high,
      low: marketData.low,
      close: marketData.close,
      optInTimePeriod: 9,
    },
  })
);

console.log(
  talib.execute({
    name: "SMA",
    startIdx: 0,
    endIdx: marketData.close.length - 1,
    params: {
      inReal: marketData.close,
      optInTimePeriod: 180,
    },
  })
);

talib.execute(
  {
    name: "ADX",
    startIdx: 0,
    endIdx: marketData.close.length - 1,
    params: {
      high: marketData.high,
      low: marketData.low,
      close: marketData.close,
      optInTimePeriod: 9,
    },
  },
  (error, result) => {
    if (error) {
      console.error(error.message);
    } else {
      console.log(result);
    }
  }
);
