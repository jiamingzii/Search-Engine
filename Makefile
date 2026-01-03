CXX = g++
CXXFLAGS = -std=c++17 -Wall -g -O2

# 路径配置
INC_DIR = include
SRC_DIR = src
OBJ_DIR = obj

# 第三方库路径（根据实际安装位置修改）
JIEBA_INC = ../cppjieba
WFREST_INC = /usr/local/include
WFREST_LIB = /usr/local/lib

# 编译选项
INCLUDES = -I$(INC_DIR) -I$(JIEBA_INC)
LIBS = -lwfrest -lworkflow -lpthread -llog4cpp

# 源文件
SRCS = $(wildcard $(SRC_DIR)/*.cc)
OBJS = $(patsubst $(SRC_DIR)/%.cc, $(OBJ_DIR)/%.o, $(SRCS))

# 目标文件
TARGET = search_engine

.PHONY: all clean dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p conf
	@mkdir -p data
	@mkdir -p logs

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# 运行
run-build: $(TARGET)
	./$(TARGET) build

run-server: $(TARGET)
	./$(TARGET) server

# 依赖关系
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.cc $(INC_DIR)/Configuration.h $(INC_DIR)/SplitTool.h \
                   $(INC_DIR)/PageLib.h $(INC_DIR)/InvertIndex.h $(INC_DIR)/SearchServer.h \
                   $(INC_DIR)/DictProducer.h $(INC_DIR)/KeywordRecommender.h $(INC_DIR)/Logger.h
$(OBJ_DIR)/Configuration.o: $(SRC_DIR)/Configuration.cc $(INC_DIR)/Configuration.h $(INC_DIR)/Logger.h
$(OBJ_DIR)/SplitTool.o: $(SRC_DIR)/SplitTool.cc $(INC_DIR)/SplitTool.h $(INC_DIR)/Logger.h
$(OBJ_DIR)/WebPage.o: $(SRC_DIR)/WebPage.cc $(INC_DIR)/WebPage.h $(INC_DIR)/SplitTool.h
$(OBJ_DIR)/PageLib.o: $(SRC_DIR)/PageLib.cc $(INC_DIR)/PageLib.h $(INC_DIR)/WebPage.h $(INC_DIR)/Logger.h
$(OBJ_DIR)/PageLibPreprocessor.o: $(SRC_DIR)/PageLibPreprocessor.cc $(INC_DIR)/PageLibPreprocessor.h $(INC_DIR)/Logger.h
$(OBJ_DIR)/InvertIndex.o: $(SRC_DIR)/InvertIndex.cc $(INC_DIR)/InvertIndex.h $(INC_DIR)/WebPage.h $(INC_DIR)/Logger.h
$(OBJ_DIR)/SearchServer.o: $(SRC_DIR)/SearchServer.cc $(INC_DIR)/SearchServer.h $(INC_DIR)/InvertIndex.h \
                           $(INC_DIR)/LRUCache.h $(INC_DIR)/DictProducer.h $(INC_DIR)/KeywordRecommender.h $(INC_DIR)/Logger.h
$(OBJ_DIR)/DictProducer.o: $(SRC_DIR)/DictProducer.cc $(INC_DIR)/DictProducer.h $(INC_DIR)/SplitTool.h $(INC_DIR)/Logger.h
$(OBJ_DIR)/KeywordRecommender.o: $(SRC_DIR)/KeywordRecommender.cc $(INC_DIR)/KeywordRecommender.h $(INC_DIR)/DictProducer.h
$(OBJ_DIR)/Logger.o: $(SRC_DIR)/Logger.cc $(INC_DIR)/Logger.h
