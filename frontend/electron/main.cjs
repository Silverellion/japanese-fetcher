const { app, BrowserWindow, globalShortcut } = require("electron");
const path = require("path");
const isDev = process.env.NODE_ENV !== "production";

process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = "true";
app.commandLine.appendSwitch("disable-features", "Autofill");

function createWindow() {
  const mainWindow = new BrowserWindow({
    width: 1280,
    height: 720,
    show: false,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, "preload.cjs"),
    },
  });

  mainWindow.once("ready-to-show", () => {
    mainWindow.show();
  });

  if (isDev) {
    mainWindow.loadURL("http://localhost:5173");
    mainWindow.webContents.openDevTools();
    mainWindow.webContents.session.clearCache();
  } else {
    mainWindow.loadFile(path.join(__dirname, "../dist/index.html"));
  }

  globalShortcut.register("Control+F", () => {
    if (mainWindow.isFullScreen()) {
      mainWindow.setFullScreen(false);
    } else {
      mainWindow.setFullScreen(true);
    }
  });

  globalShortcut.register("Alt+Enter", () => {
    if (mainWindow.isFullScreen()) {
      mainWindow.setFullScreen(false);
    } else {
      mainWindow.setFullScreen(true);
    }
  });
}

app.whenReady().then(() => {
  createWindow();

  app.on("activate", function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", function () {
  if (process.platform !== "darwin") app.quit();
});

app.on("will-quit", () => {
  globalShortcut.unregisterAll();
});
