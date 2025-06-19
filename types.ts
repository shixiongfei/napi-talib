/*
 * types.ts
 *
 * Copyright (c) 2024-2025 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

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

export enum FuncUnstId {
  NONE = -1,
  ADX,
  ADXR,
  ATR,
  CMO,
  DX,
  EMA,
  HT_DCPERIOD,
  HT_DCPHASE,
  HT_PHASOR,
  HT_SINE,
  HT_TRENDLINE,
  HT_TRENDMODE,
  IMI,
  KAMA,
  MAMA,
  MFI,
  MINUS_DI,
  MINUS_DM,
  NATR,
  PLUS_DI,
  PLUS_DM,
  RSI,
  STOCHRSI,
  T3,
  ALL,
}

export enum Compatibility {
  DEFAULT,
  METASTOCK,
}

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
