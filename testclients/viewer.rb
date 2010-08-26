#!/usr/bin/ruby

require 'socket'
require 'gtk2'
require 'pp'

#$stdout.reopen("/tmp/viewer_rb.log","w")
#$stdout.sync = true
#$stderr.reopen($stdout)
#$stderr.sync = true

def parsePacket
  pp @buffer

  if(@buffer["type"] == "command")
    if(@buffer["command"] == "connect-to-renderer")
      @gtkSocket.add_id(0)
      @gtkSocket.add_id(@buffer["display-information"].to_i)
    elsif(@buffer["command"] == "disconnect-from-renderer")
      @gtkSocket.add_id(0)
    end
  elsif(@buffer["type"] == "status-change")
    case @buffer["status"]
    when "load-started"
      @statusbar.pop(@statusbarContextRendererLoad)
      @statusbar.push(@statusbarContextRendererLoad, "started loading "+@buffer["uri"])
    when "load-finished"
      @statusbar.pop(@statusbarContextRendererLoad)
      @statusbar.push(@statusbarContextRendererLoad, "done loading "+@buffer["uri"])
    when "load-failed"
      @statusbar.pop(@statusbarContextRendererLoad)
      @statusbar.push(@statusbarContextRendererLoad, "loading failed")
    when "progress-changed"
      progress = @buffer["progress"].to_f
      percentage = (progress*100).to_i

      @statusbar.pop(@statusbarContextRendererLoad)
      @statusbar.push(@statusbarContextRendererLoad, "#{percentage}% loaded")
    when "hovering-over-link"
      @statusbar.pop(@statusbarContextRendererHover)
      @statusbar.push(@statusbarContextRendererHover, @buffer["uri"])      
    when "not-hovering-over-link"
      @statusbar.pop(@statusbarContextRendererHover)
    end
  elsif(@buffer["type"] == "ack" && @buffer["id"] == "0")
    @socket.write("type = connection-management\ncommand = register-view\nview-id = view00\ndisplay-information = #{@gtkSocket.id}\ndisplay-information-type = XEMBED\nid = 1\n\n")
  end
    
  if(@buffer["id"])
    @socket.write("type = ack\nid = #{@buffer["id"]}\n\n")
  end

  @buffer = Hash.new
end

# GUI elements
@gtkSocket = Gtk::Socket.new
@statusbar = Gtk::Statusbar.new
@vbox = Gtk::VBox.new
@window = Gtk::Window.new

@window.signal_connect("destroy"){Gtk.main_quit}
@gtkSocket.signal_connect("plug-added") do 
  puts "plug was added, new window: #{@gtkSocket.plug_window}"
  @window.show_all
end

@vbox.pack_start(@gtkSocket, true, true)
@vbox.pack_end(@statusbar, false, false)
@window.add(@vbox)
@window.show_all

# init statusbar context
@statusbarContextLocal = @statusbar.get_context_id("23")
@statusbarContextServer = @statusbar.get_context_id("server")
@statusbarContextRendererLoad = @statusbar.get_context_id("renderer-load")
@statusbarContextRendererHover = @statusbar.get_context_id("renderer-hover")

@statusbar.push(@statusbarContextLocal, "Welcome to gtk-viewer on yonderboy 0.0")
# TEMP END

# IPC socket stuff
@socketFile = ARGV[0]
@socket = UNIXSocket.open(@socketFile)

# read buffer
@buffer = Hash.new

@socket.write("type = connection-management\ncommand = initialize\nid = 0\nclient-name = ruby-test\ncan-display-stati\n\n")

@thread = Thread.new {
  Kernel.loop do
    if(@socket.eof?)
      exit(0)
    end
    line = @socket.readline
    if(line == "\n")
      parsePacket()
    elsif(line =~ /^(.*)=(.*)\n$/)
      key = $1.strip
      value = $2.strip
      @buffer[key] = value
    end
  end
}

Gtk.main

@socket.close

@thread.join
