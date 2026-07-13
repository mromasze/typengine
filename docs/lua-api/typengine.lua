---@meta
-- Typengine Lua API definitions for the Lua Language Server (sumneko).
-- Point your .luarc.json at this directory:
--   { "workspace.library": ["path/to/typengine/docs/lua-api"],
--     "runtime.version": "Lua 5.4" }
-- Every function here is implemented in C++ (src/Script.cpp); this file only
-- teaches the editor the types.

---@class engine
engine = {}

---Print a message to the game console/stdout.
---@param msg string
function engine.log(msg) end

---Seconds since the script host started.
---@return number
function engine.time() end

---Load and run a script file (relative to the script root). The file is
---tracked for hot-reload: saving it re-runs it while the game is running.
---@param relPath string e.g. "quests/tutorial.lua"
---@return boolean ok false on syntax/runtime error (error is logged)
function engine.load(relPath) end

---Register an event handler. Handlers are keyed: re-registering with the same
---(event, key) replaces the old handler, which keeps hot-reload idempotent.
---Built-in events: "script_reloaded" {file}. Games emit their own.
---@param event string
---@param key string|fun(payload: table) handler key, or the handler itself
---@param fn? fun(payload: table) handler (when a key was given)
function engine.on(event, key, fn) end

---Remove a handler registered with engine.on.
---@param event string
---@param key? string defaults to "default"
function engine.off(event, key) end

---Fire an event to all registered handlers (C++ systems can also emit).
---@param event string
---@param payload? table
function engine.emit(event, payload) end

---Call fn once after `seconds` (game time, unaffected by hot-reload).
---@param seconds number
---@param fn fun()
function engine.after(seconds, fn) end

---@class Color
---@field r integer 0-255
---@field g integer 0-255
---@field b integer 0-255
---@field a? integer 0-255, default 255

---@class ButtonDef
---@field x number
---@field y number
---@field w number
---@field h number
---@field text? string
---@field text_scale? number default 2
---@field color? Color background color
---@field on_click? fun()

---@class LabelDef
---@field x number
---@field y number
---@field text string
---@field scale? number default 2 (glyphs are 5x7 px before scaling)
---@field color? Color

---@class BarDef
---@field x number
---@field y number
---@field w number
---@field h number
---@field value? number 0..1 fill fraction, default 1
---@field fill? Color

---@class UIPanel
local UIPanel = {}
function UIPanel:show() end
function UIPanel:hide() end
---@return boolean
function UIPanel:visible() end
---Remove all widgets. Typical pattern: clear + rebuild when data changes.
function UIPanel:clear() end
---@param def ButtonDef
function UIPanel:button(def) end
---@param def LabelDef
function UIPanel:label(def) end
---@param def BarDef
function UIPanel:bar(def) end

---@class ui
ui = {}

---Get or create a named screen-space panel owned by the script host.
---Calling with the same name returns the same panel (hot-reload friendly).
---@param name string
---@return UIPanel
function ui.panel(name) end

---@class input
input = {}

---Key held down this frame. Names: "W","A","S","D","UP","DOWN","LEFT","RIGHT",
---"SHIFT","CTRL","SPACE","ESC","ENTER","TAB","E","Q","F","R","I","1".."5".
---@param key string
---@return boolean
function input.is_down(key) end

---Key went down this frame (edge).
---@param key string
---@return boolean
function input.was_pressed(key) end
