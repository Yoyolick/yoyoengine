/*
    This file is a part of yoyoengine. (https://github.com/yoyolick/yoyoengine)
    Copyright (C) 2023  Ryan Zmuda

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef YE_EDITOR_SETTINGS_UI_H
#define YE_EDITOR_SETTINGS_UI_H

void editor_settings_ui_init();

void editor_settings_ui_shutdown();

void ye_editor_paint_project_settings(struct nk_context *ctx);
void ye_editor_paint_project(struct nk_context *ctx);

#endif // YE_EDITOR_SETTINGS_UI_H