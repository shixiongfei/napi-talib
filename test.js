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

const talib = require(".");

console.log(talib.getFunctionGroups());

console.log(talib.getFunctions());

console.log(talib.explain("MACDEXT"));

console.log(talib.execute({ name: "ADX", startIdx: 0, endIdx: 0 }));

console.log(
  talib.execute({}, (error, result) => {
    if (error) {
      console.error(error instanceof Error ? error.message : error);
    } else {
      console.log(result);
    }
  })
);
