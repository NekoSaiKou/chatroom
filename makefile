CC	=	/usr/bin/g++-7
CFLAGS =

BIN_DIR = bin
BUILD_DIR = obj
SRC_DIR = src

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
INCLUDES := ./$(SRC_DIR)
SERVER_OBJ := $(BUILD_DIR)/server.o $(BUILD_DIR)/packet.o
CLIENT_OBJ := $(BUILD_DIR)/client.o $(BUILD_DIR)/packet.o
DEPS := $(SOURCES:%.cpp=%.d)

all: $(BIN_DIR)/server $(BIN_DIR)/client

$(BIN_DIR)/server: $(SERVER_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $(SERVER_OBJ) -pthread

$(BIN_DIR)/client: $(CLIENT_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $(CLIENT_OBJ) -pthread

-include $(DEPS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@$(CC) -MM $< > $(BUILD_DIR)/$*.d
	$(CC) -I$(INCLUDES) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BUILD_DIR)/* $(BIN_DIR)/*