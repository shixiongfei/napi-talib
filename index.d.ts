/*
 * index.d.ts
 *
 * Copyright (c) 2024 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

export * from "./types.js";
import { FuncUnstId } from "./types.js";

export type InputParameterType = "price" | "real" | "integer";

export type InputFlags =
  | "open"
  | "high"
  | "low"
  | "close"
  | "volume"
  | "openinterest"
  | "timestamp";

export type InputParameterInfo = {
  name: string;
  type: InputParameterType;
  flags: InputFlags[];
};

export type OptInputParameterType =
  | "real_range"
  | "real_list"
  | "integer_range"
  | "integer_list";

export type OptInputFlags = "percent" | "degree" | "currency" | "advanced";

export type OptInputParameterInfo = {
  name: string;
  displayName: string;
  defaultValue: number;
  hint: string;
  type: OptInputParameterType;
  flags: OptInputFlags[];
};

export type OutputParameterType = "real" | "integer";

export type OutputFlags =
  | "line"
  | "line_dot"
  | "line_dash"
  | "dot"
  | "histogram"
  | "pattern_bool"
  | "pattern_bull_bear"
  | "pattern_strength"
  | "positive"
  | "negative"
  | "zero"
  | "limit_upper"
  | "limit_lower";

export type OutputParameterInfo = {
  name: string;
  type: OutputParameterType;
  flags: OutputFlags;
};

export type FuncInfo = {
  name: string;
  group: string;
  hint: string;
  inputs: InputParameterInfo[];
  optInputs: OptInputParameterInfo[];
  outputs: OutputParameterInfo[];
};

export type FuncParam = {
  name: string;
  startIdx: number;
  endIdx: number;
  params: { [name: string]: number[] | number };
};

export type FuncResult = {
  begIndex: number;
  nbElement: number;
  results: { [name: string]: number[] };
};

/** Get functions including grouping */
export declare function getFunctionGroups(): Record<string, string[]>;

/** Get functions */
export declare function getFunctions(): string[];

/** Set unstable period function */
export declare function setUnstablePeriod(
  funcId: FuncUnstId,
  period: number
): void;

/** Get function infomation */
export declare function explain(funcName: string): FuncInfo;

/** Execute sync function */
export declare function execute(param: FuncParam): FuncResult;

/** Execute async function */
export declare function execute(
  param: FuncParam,
  callback: (error: Error, result: FuncResult) => void
): void;

/** Get TA-Lib version */
export declare function version(): string;
