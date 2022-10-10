# webserv

Project developed by onapoli- & pmira-pe students from 42.

**webserv** aims to teach how _web servers_ work. So this project is like an Nginx made from zero using C++98 with no external libraries and targetting UNIX operating systems.

## Requirements

- UNIX environment (https://en.wikipedia.org/wiki/Unix)
- GNU Make (https://www.gnu.org/software/make/)

## How to run it

- For **default configuration**. Inside the repository directory run:
	```
	make && make start
	````
	This will automatically compile the project into `webserv` executable and execute it with a config file that is based on `tests/example_config.json`. A directory `tests/www/` is generated containing the root directory for the web server and the website content.

- For **alternative default configuration**. Inside the repository directory run:
	```
	make
	````
	Then run:
	```
	./webserv
	```
	This will run the server using `default.json`, located at `.default/` directory, as the server configuration file, and having `.default` directory as the server's root.

- For **custom configuration**. Create a custom config file in `json` format (check out `tests/example_config.json` for reference). Inside the repository directory run:
	```
	make
	````
	Then run:
	```
	./webserv path_to_your_custom_config.json
	```

Once the server is running, open a web browser, go to `http://configuredhost:configuredport` and visit the different configured routes (locations).

These config files have strict rules. Check `tests/example_config.json` to have an idea of the available options, and feel free to modify it.

## How to stop it

- To **stop the server**. Inside the repository directory press:
	```
	Ctrl + C
	```

- To **clean the repository folder** removing `webserv` executable, object files (.o), `tests/www` directory and generated config file. Inside the repository directory run:
	```
	make fclean
	```
