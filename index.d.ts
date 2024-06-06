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

export enum MAType {
  SMA,
  EMA,
  WMA,
  DEMA,
  TEMA,
  TRIMA,
  KAMA,
  MAMA,
  T3,
}

export type FuncParam = {
  name: string;
  startIdx: number;
  endIdx: number;
  [parameter: string]: number[] | number;
};

export type FuncResult = {
  begIndex: number;
  nbElement: number;
  result: number[];
};

/** Get functions including grouping */
export declare function getFunctionGroups(): Record<string, string[]>;

/** Get functions */
export declare function getFunctions(): string[];

/** Get function infomation */
export declare function explain(funcName: string): FuncInfo;

/** Execute function */
export declare function execute(param: FuncParam): FuncResult;
