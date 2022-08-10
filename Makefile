ROOT = ./sources

NAME = webserv
CXX = clang++
CXXFLAGS = -Wall -Werror -Wextra -Iincludes -fsanitize=address -g3 -std=c++98 -pedantic

SRC =	main.cpp \
		$(ROOT)/Config.cpp \
		$(ROOT)/HttpInfo.cpp \
		$(ROOT)/Data.cpp \
		$(ROOT)/Request.cpp \
		$(ROOT)/CgiHandler.cpp \
		$(ROOT)/FileHandler.cpp \
		$(ROOT)/Monitor.cpp \
		$(ROOT)/FdTable.cpp \
		$(ROOT)/Processor.cpp \
		$(ROOT)/GetProcessor.cpp \
		$(ROOT)/DeleteProcessor.cpp \
		$(ROOT)/UrlParser.cpp \
		$(ROOT)/utils.cpp \
		$(ROOT)/Response.cpp \
		$(ROOT)/Server.cpp

OBJ =	$(SRC:.cpp=.o)

MSG = Default commit message
#MSG="message" to change message


all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

start:
	@sh server_content_setup.sh
	@sed s#WEBSERV_PATH#$(PWD)#g tests/example_config.json \
		> tests/tmp_config.json
	./webserv tests/tmp_config.json

response:
	$(CXX) $(CXXFLAGS) $(ROOT)/Response.cpp ./tests/response.cpp -o response

request:
	$(CXX) $(CXXFLAGS) $(ROOT)/Request.cpp ./tests/requestTest.cpp -o request

config:
	$(CXX) $(CXXFLAGS) $(ROOT)/Config.cpp ./tests/config.cpp -o config

config_multi:
	$(CXX) $(CXXFLAGS) $(ROOT)/Config.cpp ./tests/multi_config.cpp -o config
	@touch sample_test_config.json
	@./config tests/multi_config.json sample_test_config.json
	@rm sample_test_config.json config

monitor:
	$(CXX) $(CXXFLAGS) $(ROOT)/Monitor.cpp ./tests/monitor.cpp -o monitor
	@./monitor
	@rm monitor

url_parser:
	$(CXX) $(CXXFLAGS) $(ROOT)/UrlParser.cpp ./tests/urlParser.cpp -o url_parser
	@./url_parser
	@rm url_parser

utils:
	$(CXX) $(CXXFLAGS) $(ROOT)/utils.cpp ./tests/utils.cpp -o utils
	@./utils
	@rm utils

clean:
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(NAME)
	@rm -f response request config tests/tmp_config.json
	@rm -rf tests/www

re: fclean $(NAME)

git: fclean
	git add .
	git commit -m "$(MSG)"
	git push

.PHONY: all start response request config config_multi monitor url_parser \
				utils clean fclean re
