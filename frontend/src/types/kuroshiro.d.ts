declare module "kuroshiro" {
  export default class Kuroshiro {
    constructor();

    init(analyzer: any): Promise<void>;

    convert(
      text: string,
      options?: {
        to?: "hiragana" | "katakana" | "romaji" | "html" | "ascii";
        mode?: "normal" | "spaced" | "okurigana" | "furigana";
        romajiSystem?: "nippon" | "passport" | "hepburn";
        delimiter_start?: string;
        delimiter_end?: string;
      }
    ): Promise<string>;
  }
}

declare module "kuroshiro-analyzer-kuromoji" {
  export default class KuromojiAnalyzer {
    constructor(options?: { dictPath?: string });
    init(): Promise<void>;
    parse(text: string): Promise<any>;
  }
}
