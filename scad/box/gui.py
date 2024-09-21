"""GUI for interactive control of OpenSCAD test renders."""
from pathlib import Path
import os
import re
import subprocess
import threading

import gi

gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")
from gi.repository import Gdk, GLib, Gtk


class BoolParam:
    """Boolean parameter"""
    def __init__(self, match, parent):
        self._varname = match.group(1)
        self._value = match.group(2) == 'true'
        self._parent = parent
        self._widget = Gtk.ToggleButton(label=self._varname)
        self._widget.set_active(self._value)
        self._widget.connect("toggled", self._on_toggled)

    @property
    def widget(self):
        return self._widget

    @property
    def varname(self):
        return self._varname

    def write(self, stream):
        stream.write('// :GUI:\n{} = {};\n'.format(
            self._varname, 'true' if self._value else 'false'))

    def _on_toggled(self, button):
        self._value = button.get_active()
        self._parent.write()


class FloatParam:
    """Float parameter"""
    def __init__(self, match, parent):
        self._minval = float(match.group(1))
        self._maxval = float(match.group(2))
        self._step = float(match.group(3))
        self._varname = match.group(4)
        self._value = float(match.group(5))
        self._parent = parent
        self._widget = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL,
                                                self._minval, self._maxval, self._step)
        self._widget.set_value(self._value)
        self._widget.connect("value_changed", self._on_changed)

    @property
    def widget(self):
        return self._widget

    @property
    def varname(self):
        return self._varname

    def write(self, stream):
        stream.write(f'// :GUI:float:{self._minval}:{self._maxval}:{self._step}:\n'
                     f'{self._varname} = {self._value};\n')

    def _on_changed(self, widget):
        self._value = widget.get_value()
        self._parent.write()


class IntParam:
    """Integer parameter"""
    def __init__(self, match, parent):
        self._minval = int(match.group(1))
        self._maxval = int(match.group(2))
        self._varname = match.group(3)
        self._value = int(match.group(4))
        self._parent = parent
        self._widget = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL,
                                                self._minval, self._maxval, 1)
        self._widget.set_value(self._value)
        self._widget.connect("value_changed", self._on_changed)

    @property
    def widget(self):
        return self._widget

    @property
    def varname(self):
        return self._varname

    def write(self, stream):
        stream.write(f'// :GUI:int:{self._minval}:{self._maxval}:\n'
                     f'{self._varname} = {self._value};\n')

    def _on_changed(self, widget):
        self._value = int(widget.get_value())
        self._parent.write()


_PATTERNS = ((re.compile(r'\/\/ \:GUI\:\n(\w+)\s*=\s*(true|false)\s*;\s*\n'),
              BoolParam),
             (re.compile(r':GUI:float:(\d+(?:\.\d*)?):(\d+(?:\.\d*)?):(\d+(?:\.\d*)?):\n'
                         r'(\w+)\s*=\s*(\d+(?:\.\d*)?)\s*;\s*\n'),
              FloatParam),
             (re.compile(r':GUI:int:(\d+):(\d+):\n'
                         r'(\w+)\s*=\s*(\d+?)\s*;\s*\n'),
              IntParam))


def parse(text, parent):
    """Find all parameters iterate over objects to represent them."""
    for pattern, klass in _PATTERNS:
        for match in pattern.finditer(text):
            param = klass(match, parent)
            yield param


class GUIWindow(Gtk.ApplicationWindow):
    """GUI Window to set parameters."""
    def __init__(self):
        name = Path.cwd().name
        super().__init__(title=name)
        self._params = []
        self.set_border_width(10)
        vbox = Gtk.VBox(spacing=6)
        self.add(vbox)
        hbox1 = Gtk.Box(spacing=6)
        vbox.pack_start(hbox1, True, True, 0)
        openscad_button = Gtk.Button(label='openscad')
        openscad_button.connect("clicked", self._launch_openscad)
        hbox1.pack_start(openscad_button, True, True, 0)
        render_button = Gtk.Button(label='render')
        render_button.connect("clicked", self._render)
        hbox1.pack_start(render_button, True, True, 0)
        prusa_slider_button = Gtk.Button(label='prusa-slicer')
        prusa_slider_button.connect("clicked", self._launch_slicer)
        hbox1.pack_start(prusa_slider_button, True, True, 0)

        with open('gui.scad') as infile:
            for param in parse(infile.read(), self):
                vbox.pack_start(param.widget, True, True, 0)
                self._params.append(param)


    def write(self):
        with open('gui.scad.new', 'w') as outfile:
            for param in self._params:
                param.write(outfile)
        os.rename('gui.scad', 'gui.scad.old')
        os.rename('gui.scad.new', 'gui.scad')

    def _render(self, _button):

        window = Gdk.get_default_root_window()
        window.set_cursor(Gdk.Cursor(Gdk.CursorType.WATCH))
        print("> Rendering started..")
        def _restore_cursor():
            window.set_cursor(Gdk.Cursor(Gdk.CursorType.ARROW))

        def _render():
            subprocess.call(["./render.sh"])
            GLib.idle_add(_restore_cursor)
            print("> Rendering finished.")

        thread = threading.Thread(target=_render)
        thread.daemon = True
        thread.start()

    def _launch_openscad(self, _button):
        subprocess.call("openscad&", shell=True)

    def _launch_slicer(self, _button):
        subprocess.call("prusa-slicer&", shell=True)


def main():
    """Command-line-interface."""
    win = GUIWindow()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
