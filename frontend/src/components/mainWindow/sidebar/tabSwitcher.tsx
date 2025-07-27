import React from "react";
import { type SubtitleState } from "../../../App";
import { type Tab } from "../../../types";
import Record from "../tabs/record/record";
import Transcripts from "../tabs/transcripts";
import Audios from "../tabs/audios";
import Settings from "../tabs/settings";

type TabSwitcherProps = {
  currentTab: Tab;
} & Pick<SubtitleState, "subtitles" | "setSubtitles" | "backendReady" | "setBackendReady">;

const TabSwitcher: React.FC<TabSwitcherProps> = ({
  currentTab,
  subtitles,
  setSubtitles,
  backendReady,
  setBackendReady,
}) => {
  switch (currentTab) {
    case "record":
      return (
        <Record
          subtitles={subtitles}
          setSubtitles={setSubtitles}
          backendReady={backendReady}
          setBackendReady={setBackendReady}
        />
      );
    case "transcripts":
      return <Transcripts />;
    case "audios":
      return <Audios />;
    case "settings":
      return <Settings />;
  }
};

export default TabSwitcher;
