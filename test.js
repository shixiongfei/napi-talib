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
const { ADX } = require("./functions");
const { SMA } = require("./promises.js");

/*
 * In real projects, it can be written like this
 *
 * import talib from "napi-talib";
 * import { ADX } from "napi-talib/funcs";
 * import { SMA } from "napi-talib/funcs/promise";
 *
 */

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
  ADX(marketData.high, marketData.low, marketData.close, {
    optInTimePeriod: 9,
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

SMA(marketData.close, { optInTimePeriod: 180 }).then((sma) => {
  console.log(sma);
});
