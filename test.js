/*
 * test.js
 *
 * Copyright (c) 2024-2025 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

import fs from "node:fs";
import talib from "./index.js";
import { ADX } from "./functions.js";
import { SMA, EMA } from "./promises.js";

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

console.log(talib.setUnstablePeriod(talib.FuncUnstId.MAMA, 14));

console.log(talib.setCompatibility(talib.Compatibility.METASTOCK));

console.log(talib.explain("ADX"));

console.log(
  // Synchronous call
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
  // Synchronous call
  ADX(marketData.high, marketData.low, marketData.close, {
    optInTimePeriod: 9,
  })
);

// Asynchronous call
talib.execute(
  {
    name: "SMA",
    startIdx: 0,
    endIdx: marketData.close.length - 1,
    params: {
      inReal: marketData.close,
      optInTimePeriod: 180,
    },
  },
  (error, results) => {
    if (error) {
      console.error(error.message);
    } else {
      console.log(results);
    }
  }
);

// Parallel computing
Promise.all([
  SMA(marketData.close, { optInTimePeriod: 5 }),
  EMA(marketData.close, { optInTimePeriod: 5 }),
]).then(([sma, ema]) => {
  console.log("SMA:", sma);
  console.log("EMA:", ema);
});
