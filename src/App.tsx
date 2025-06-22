import "./global.css";
import StickyBox from "./components/stickyWindow/stickyBox";

function App() {
  return (
    <>
      <div className="bg-[rgb(30,30,30)] h-screen w-screen min-h-screen flex">
        <StickyBox></StickyBox>
      </div>
    </>
  );
}

export default App;
