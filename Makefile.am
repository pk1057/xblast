if XBSNDSRV
soundbinaries =  xbsndsrv
else
soundbinaries =  
endif

bin_PROGRAMS =  ${soundbinaries} xblast

xblast_SOURCES=xblast.c util.c str_util.c intro.c introdat.c map.c sprite.c \
	color.c status.c player.c action.c bomb.c event.c image.c\
	shrink.c func.c info.c ini_file.c atom.c scramble.c \
	demo.c debug.c level.c random.c \
	game_local.c game_demo.c game_server.c game_client.c game.c\
	menu.c menu_player.c menu_level.c menu_control.c menu_game.c\
	menu_network.c menu_extras.c	mi_tool.c mi_base.c\
	mi_button.c mi_color.c mi_combo.c mi_cyclic.c\
	mi_host.c mi_int.c mi_keysym.c mi_label.c mi_player.c\
	mi_string.c 	mi_tag.c mi_toggle.c mi_map.c mi_stat.c	client.c\
	server.c network.c com.c browse.c	com_to_server.c\
	com_listen.c com_to_client.c com_stream.c 	com_dg_client.c\
	com_dg_server.c com_dgram.c 	com_query.c com_browse.c\
	com_reply.c com_base.c 	net_socket.c net_tele.c\
	net_dgram.c 	central.c com_newgame.c com_to_central.c\
	com_from_central.c com_central.c user.c dat_rating.c \
	cfg_main.c cfg_level.c cfg_player.c cfg_game.c\
	cfg_control.c 	cfg_stat.c cfg_demo.c cfg_xblast.c\
	x11_common.c x11_event.c x11_atom.c x11_config.c\
	x11_msgbox.c	x11c_init.c x11c_image.c x11c_text.c\
	x11c_tile.c x11c_sprite.c 	x11c_pixmap.c x11_sound.c\
	x11_socket.c x11_joystick.c  bot.c xblast.h util.h \
	str_util.h intro.h introdat.h map.h sprite.h 	color.h \
	status.h player.h action.h bomb.h event.h image.h \
	shrink.h func.h info.h ini_file.h atom.h scramble.h \
	demo.h debug.h level.h random.h \
	game_local.h game_demo.h game_server.h game_client.h game.h\
	menu.h menu_player.h menu_level.h menu_control.h menu_game.h\
	menu_network.h menu_extras.h	mi_tool.h mi_base.h\
	mi_button.h mi_color.h mi_combo.h mi_cyclic.h\
	mi_host.h mi_int.h mi_keysym.h mi_label.h mi_player.h\
	mi_string.h 	mi_tag.h mi_toggle.h mi_map.h mi_stat.h	client.h\
	server.h network.h com.h browse.h	com_to_server.h\
	com_listen.h com_to_client.h com_stream.h \
	com_dg_client.h com_dg_server.h com_dgram.h \
	com_query.h com_browse.h com_reply.h com_base.h \
	net_socket.h net_tele.h net_dgram.h 	central.h\
	com_newgame.h com_to_central.h com_from_central.h com_central.h\
	user.h dat_rating.h 	cfg_main.h cfg_level.h cfg_player.h\
	cfg_game.h cfg_control.h 	cfg_stat.h cfg_demo.h cfg_xblast.h\
	x11_common.h x11_event.h  x11_config.h  x11c_image.h\
	x11c_text.h x11c_tile.h x11c_sprite.h 	x11c_pixmap.h\
	x11_sound.h x11_socket.h x11_joystick.h  bot.h shrinkdat.h\
	version.h  x11c_sprite.h socket.h  gui.h geom.h config.h snd.h\
	common.h menu_layout.h chat.h chat.c menu_edit.c menu_edit.h version.c

EXTRA_DIST =  xblast.man autogen.sh Makefile.w32 Makefile.windows *.ico \
	*.coff xblast.rc w32*.h w32*.c time.c timeval.h Makefile.dist Imakefile\
	image/misc/*.ppm image/misc/*.pbm  image/explosion/*.pbm \
	image/explosion/*.ppm image/score/*.epm image/score/*.ppm \
	image/sprite/*.pbm image/sprite/*.ppm image/block/*.ppm \
	image/sprite/*.epm level/*.xal sounds/*.raw

xbsndsrv_SOURCES = xbsndsrv.c snd.h 

game_datadir=@game_datadir@


install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/image; then \
	  $(mkinstalldirs)     $(game_datadir); \
	  $(mkinstalldirs) $(game_datadir)/image; \
	  cp -r $(srcdir)/image/* $(game_datadir)/image;\
	fi; \
	if test -d $(srcdir)/level; then \
	  $(mkinstalldirs) $(game_datadir)/ ; \
	  $(mkinstalldirs) $(game_datadir)//level; \
	  cp -r $(srcdir)/level/* $(game_datadir)//level;\
	fi; \
	if test -d $(srcdir)/sounds; then \
	  $(mkinstalldirs) $(game_datadir)/ ; \
	  $(mkinstalldirs) $(game_datadir)//sounds; \
	  cp -r $(srcdir)/sounds/* $(game_datadir)//sounds;\
	fi;
