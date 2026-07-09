    (function () {
      const savedTheme = localStorage.getItem("qrp-dashboard-theme");
      const savedStyle = localStorage.getItem("qrp-dashboard-style");
      const systemTheme = window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
      document.documentElement.dataset.theme = savedTheme || systemTheme;
      document.documentElement.dataset.palette = savedStyle || "institutional";
    }());
