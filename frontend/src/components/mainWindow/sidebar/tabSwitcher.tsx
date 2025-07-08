import React from "react";
import { type Tab } from "../../../types";
import Record from "../tabs/record/record";
import Transcripts from "../tabs/transcripts";
import Audios from "../tabs/audios";
import Settings from "../tabs/settings";

const TabSwitcher: React.FC<{ currentTab: Tab }> = ({ currentTab }) => {
  switch (currentTab) {
    case "record":
      return <Record />;
    case "transcripts":
      return <Transcripts />;
    case "audios":
      return <Audios />;
    case "settings":
      return <Settings />;
  }
};

export default TabSwitcher;
