--Minetest
--Copyright (C) 2014 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--This program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

--------------------------------------------------------------------------------
io.write(" breslin ")
local function get_formspec(tabview, name, tabdata)
	-- Update the cached supported proto info,
	-- it may have changed after a change by the settings menu.
	common_update_cached_supp_proto()

	if not tabdata.search_for then
		tabdata.search_for = ""
	end

	local retval =
		-- Address / Port
		"label[7.75,-0.25;" .. fgettext("Address / Port") .. "]" ..
		"field[8,0.65;3.25,0.5;te_address;;" ..
			core.formspec_escape(core.settings:get("address")) .. "]" ..
		"field[11.1,0.65;1.4,0.5;te_port;;" ..
			core.formspec_escape(core.settings:get("remote_port")) .. "]" ..

		-- Name / Password
		"label[7.75,0.95;" .. fgettext("Name / Password") .. "]" ..
		"field[8,1.85;2.9,0.5;te_name;;" ..
			core.formspec_escape(core.settings:get("name")) .. "]" ..
		"pwdfield[10.73,1.85;1.77,0.5;te_pwd;]" ..

		-- Description Background
		"box[7.73,2.25;4.25,2.6;#999999]"..

		-- Connect
		"button[9.88,4.9;2.3,1;btn_mp_connect;" .. fgettext("Connect") .. "]"

	return retval
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)

	if fields.te_name then
		gamedata.playername = fields.te_name
		core.settings:set("name", fields.te_name)
	end

	if fields.key_up or fields.key_down then
		local fav_idx = core.get_table_index("favourites")

		if fav_idx then
			if fields.key_up and fav_idx > 1 then
				fav_idx = fav_idx - 1
			elseif fields.key_down and fav_idx < #menudata.favorites then
				fav_idx = fav_idx + 1
			end
		else
			fav_idx = 1
		end

		local address = fav.address
		local port    = fav.port
		gamedata.serverdescription = fav.description
		if address and port then
			core.settings:set("address", address)
			core.settings:set("remote_port", port)
		end

		tabdata.fav_selected = fav_idx
		return true
	end

	if (fields.btn_mp_connect or fields.key_enter)
			and fields.te_address ~= "" and fields.te_port then
		gamedata.playername = fields.te_name
		gamedata.password   = fields.te_pwd
		gamedata.address    = fields.te_address
		gamedata.selected_world = 0

		io.write(gamedata.playername)
		io.write(gamedata.password)

		core.settings:set("address",     fields.te_address)
		core.settings:set("remote_port", fields.te_port)

		core.start()
		return true
	end
	return false
end

local function on_change(type, old_tab, new_tab)
	if type == "LEAVE" then return end
	asyncOnlineFavourites()
end

--------------------------------------------------------------------------------
return {
	name = "online",
	caption = fgettext("Join Game"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
