-- typengine scripting example — everything here hot-reloads while running.

local clicks = 0

local function rebuild()
    local p = ui.panel("demo")
    p:clear()
    p:label{ x = 40, y = 40, text = "Hello from Lua!", scale = 3 }
    p:label{ x = 40, y = 80, text = "Clicks: " .. clicks, scale = 2,
             color = { r = 255, g = 220, b = 120 } }
    p:button{
        x = 40, y = 120, w = 220, h = 48, text = "Click me",
        on_click = function()
            clicks = clicks + 1
            engine.log("clicked " .. clicks .. " times")
            rebuild()
        end,
    }
    p:bar{ x = 40, y = 190, w = 220, h = 16, value = (clicks % 10) / 10,
           fill = { r = 150, g = 70, b = 220 } }
    p:show()
end

engine.on("game_started", "demo", function()
    engine.log("game_started received at t=" .. string.format("%.2f", engine.time()))
    engine.after(2.0, function() engine.log("two seconds elapsed") end)
end)

-- Rebuild on (re)load so hot-reload refreshes the UI instantly.
engine.on("script_reloaded", "demo", rebuild)
rebuild()
