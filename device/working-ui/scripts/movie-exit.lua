-- KiddOS shared video-player exit shortcut.
-- Two consecutive simultaneous Start + Select presses quit playback.

local start_down = false
local select_down = false
local chord_active = false
local chord_count = 0
local last_chord_time = -100
local double_press_window = 2.0

local function check_chord()
    if not (start_down and select_down) or chord_active then
        return
    end

    chord_active = true

    local now = mp.get_time()
    if now - last_chord_time <= double_press_window then
        chord_count = chord_count + 1
    else
        chord_count = 1
    end
    last_chord_time = now

    if chord_count >= 2 then
        mp.commandv("quit")
    end
end

local function handle_button(button, event)
    -- mpv may logically cancel a held key when the other chord key is pressed.
    -- Keep its physical held state so the simultaneous chord can be detected.
    if event.canceled then
        return
    end

    if event.event == "down" or event.event == "press" then
        if button == "start" then
            start_down = true
        else
            select_down = true
        end
        check_chord()
    elseif event.event == "up" then
        if button == "start" then
            start_down = false
        else
            select_down = false
        end
        chord_active = false
    end
end

mp.add_forced_key_binding("GAMEPAD_START", "kiddos_exit_start",
    function(event) handle_button("start", event) end,
    { complex = true })

mp.add_forced_key_binding("GAMEPAD_BACK", "kiddos_exit_select",
    function(event) handle_button("select", event) end,
    { complex = true })
