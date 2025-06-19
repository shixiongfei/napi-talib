/*
 * index.d.ts
 *
 * Copyright (c) 2024-2025 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

export * from "./types.js";
import * as types from "./types.js";

/** Get functions including grouping */
export declare function getFunctionGroups(): Record<string, string[]>;

/** Get functions */
export declare function getFunctions(): string[];

/** Set unstable period function */
export declare function setUnstablePeriod(
  funcId: types.FuncUnstId,
  period: number
): void;

/** Set compatibility function */
export declare function setCompatibility(value: types.Compatibility): void;

/** Get function infomation */
export declare function explain(funcName: string): types.FuncInfo;

/** Execute sync function */
export declare function execute(param: types.FuncParam): types.FuncResult;

/** Execute async function */
export declare function execute(
  param: types.FuncParam,
  callback: (error: Error | undefined, result: types.FuncResult) => void
): void;

/** Get TA-Lib version */
export declare function version(): string;

export default {
  getFunctionGroups,
  getFunctions,
  setUnstablePeriod,
  setCompatibility,
  explain,
  execute,
  version,
  ...types,
};
