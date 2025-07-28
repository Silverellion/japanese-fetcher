import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import App from "./App.tsx";
import StickyWindow from "./components/stickyWindow/stickyWindow";

const hash = window.location.hash;

if (hash === "#/sticky") {
  createRoot(document.getElementById("root")!).render(
    <StrictMode>
      <StickyWindow />
    </StrictMode>
  );
} else {
  createRoot(document.getElementById("root")!).render(
    <StrictMode>
      <App />
    </StrictMode>
  );
}
