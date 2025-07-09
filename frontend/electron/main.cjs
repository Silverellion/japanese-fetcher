const { app, BrowserWindow, globalShortcut, ipcMain } = require("electron");
const path = require("path");
const { spawn } = require("child_process");
const isDev = process.env.NODE_ENV !== "production";

process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = "true";
app.commandLine.appendSwitch("disable-features", "Autofill");

const backendPath = path.join(__dirname, "../../backend/cpp/x64/Debug/cpp.exe");
let backendProcess = null;
let backendReady = false;
let pendingStatusResolvers = [];

function createWindow() {
  const mainWindow = new BrowserWindow({
    width: 1280,
    height: 720,
    minWidth: 960,
    minHeight: 540,
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

function startBackend() {
  if (backendProcess) return;
  backendProcess = spawn(backendPath, [], {
    stdio: ["pipe", "pipe", "pipe"],
    windowsHide: true,
    detached: true,
  });

  backendProcess.stdout.setEncoding("utf8");
  backendProcess.stderr.setEncoding("utf8");

  backendProcess.stdout.on("data", (data) => {
    data.split(/\r?\n/).forEach((line) => {
      if (line === "recording" || line === "not-recording") {
        if (pendingStatusResolvers.length > 0) {
          const resolve = pendingStatusResolvers.shift();
          resolve(line === "recording");
        }
      }
    });
  });

  backendProcess.stderr.on("data", (data) => {
    console.error("[backend error]", data.trim());
  });

  backendProcess.on("exit", (code) => {
    console.log(`Backend exited with code ${code}`);
    backendProcess = null;
    backendReady = false;
  });

  backendReady = true;
}

function sendCommand(command) {
  if (backendProcess && backendReady) {
    backendProcess.stdin.write(command + "\n");
  }
}

// IPC handlers
ipcMain.handle("start-recording", async () => {
  startBackend();
  sendCommand("start-recording");
  return true;
});

ipcMain.handle("stop-recording", async () => {
  if (backendProcess && backendReady) {
    sendCommand("stop-recording");
    return true;
  }
  return false;
});

ipcMain.handle("get-recording-status", async () => {
  return new Promise((resolve) => {
    if (backendProcess && backendReady) {
      pendingStatusResolvers.push(resolve);
      sendCommand("get-status");
      setTimeout(() => {
        if (pendingStatusResolvers.includes(resolve)) {
          pendingStatusResolvers = pendingStatusResolvers.filter((r) => r !== resolve);
          resolve(false);
        }
      }, 2000);
    } else {
      resolve(false);
    }
  });
});

app.whenReady().then(() => {
  createWindow();
  startBackend();

  app.on("activate", function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", function () {
  if (backendProcess) {
    sendCommand("exit");
    setTimeout(() => {
      backendProcess.kill();
      backendProcess = null;
    }, 500);
  }
  if (process.platform !== "darwin") app.quit();
});

app.on("will-quit", () => {
  if (backendProcess) {
    sendCommand("exit");
    setTimeout(() => {
      backendProcess.kill();
      backendProcess = null;
    }, 500);
  }
  globalShortcut.unregisterAll();
});
