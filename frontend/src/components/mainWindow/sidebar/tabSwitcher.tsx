import React from "react";
import { type Tab } from "../../../types";
import Home from "../tabs/home";
import Transcripts from "../tabs/transcripts";
import Audios from "../tabs/audios";
import Settings from "../tabs/settings";

const TabSwitcher: React.FC<{ currentTab: Tab }> = ({ currentTab }) => {
  switch (currentTab) {
    case "home":
      return <Home />;
    case "transcripts":
      return <Transcripts />;
    case "audios":
      return <Audios />;
    case "settings":
      return <Settings />;
  }
};

export default TabSwitcher;
