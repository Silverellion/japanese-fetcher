import React from "react";
import { type Tab } from "../../../types";
import { CircleStop, FileAudio, Captions, Settings, type LucideIcon } from "lucide-react";

interface SidebarProps {
  currentTab: Tab;
  onTabChange: (tab: Tab) => void;
}
const Sidebar: React.FC<SidebarProps> = ({ currentTab, onTabChange }) => {
  // prettier-ignore
  const tabs: {key: Tab, label: string, icon: LucideIcon}[] = [
    {key: 'record',      label: 'Record',      icon: CircleStop},
    {key: 'transcripts', label: 'Transcripts', icon: Captions},
    {key: 'audios',      label: 'Saved Audio', icon: FileAudio},
    {key: 'settings',    label: 'Settings',    icon: Settings},
  ];
  return (
    <div className="bg-[rgb(20,20,20)]">
      {tabs.map((tab) => {
        const Icon = tab.icon;
        return (
          <div
            key={tab.key}
            onClick={() => onTabChange(tab.key)}
            className={`flex items-center gap-2 mx-2 my-0.5 py-1.5 ps-4 pe-6 
            rounded-[15px] cursor-pointer 
            transition-all duration-300 transform
            hover:bg-[rgb(40,40,40)] hover:translate-x-5
            ${currentTab === tab.key ? "bg-[rgb(40,40,40)]" : "bg-[rgb(20,20,20)]"}
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
