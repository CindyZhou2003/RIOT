MODULE = gesture

SRCXXEXT = cc

CXXEXFLAGS += -Wno-unused-parameter
CXXEXFLAGS += -Wno-type-limits

CFLAGS += -Wno-pedantic

# Add the tensorflow lite quantized model as a blob
BLOBS += model.tflite

include $(RIOTBASE)/Makefile.base
