CXX = g++
CXXFLAGS = -O2 -W -Wall -g `pkg-config gtkmm-2.4 --cflags`
OBJS = main.o in_mad.o common.o utils.o out_alsa.o in_vorbis.o

japlay2: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) `pkg-config gtkmm-2.4 --libs` \
		-lmad -lcrypto -lasound -lvorbisfile -lcppbencode

clean:
	rm -rf $(OBJS)
