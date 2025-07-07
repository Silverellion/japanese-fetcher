import React from "react";
import { type Tab } from "../../../types";
import {
  House,
  FileAudio,
  Captions,
  Settings,
  type LucideIcon,
} from "lucide-react";

interface SidebarProps {
  currentTab: Tab;
  onTabChange: (tab: Tab) => void;
}
const Sidebar: React.FC<SidebarProps> = ({ currentTab, onTabChange }) => {
  // prettier-ignore
  const tabs: {key: Tab, label: string, icon: LucideIcon}[] = [
    {key: 'home',        label: 'Home',        icon: House},
    {key: 'transcripts', label: 'Transcripts', icon: Captions},
    {key: 'audios',      label: 'Audios',      icon: FileAudio},
    {key: 'settings',    label: 'Settings',    icon: Settings},
  ];
  return (
    <div className="bg-[rgb(15,15,15)]">
      {tabs.map((tab) => {
        const Icon = tab.icon;
        return (
          <div
            key={tab.key}
            onClick={() => onTabChange(tab.key)}
            className={`flex items-center gap-2 py-2 ps-2 pe-5 
            hover:bg-[rgb(0,0,0)] cursor-pointer
            ${currentTab === tab.key ? "bg-[rgb(0,0,0)]" : "bg-[rgb(15,15,15)]"}
          `}
          >
            <Icon size={25} strokeWidth={1} className="text-white"></Icon>
            <span className="text-white">{tab.label}</span>
          </div>
        );
      })}
    </div>
  );
};

export default Sidebar;
