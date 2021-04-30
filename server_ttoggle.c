/**
 * Basic code to toggle text input by client and some logging for the same.
 */
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <arpa/inet.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>

const char *fname = "log.log";
const char *wmsg = "Text toggle\n";

FILE *logfile;

static void help();
static void init_log(const char *);
static void wtolog(int, const char *);
static void lstnr_cb(struct evconnlistener *, evutil_socket_t,
		     struct sockaddr *, int, void *);
static void signal_cb(evutil_socket_t, short, void *);
static void event_cb(struct bufferevent *, short, void *);
static void __rcb(struct bufferevent *, void *);

int main() {
	help();
	init_log(fname);

	struct event_base *eb;
	assert((eb = event_base_new()) != NULL);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8888);

	struct evconnlistener *lstnr;
	assert((lstnr = evconnlistener_new_bind(eb, lstnr_cb, (void *) eb,
						LEV_OPT_CLOSE_ON_FREE |
						LEV_OPT_REUSEABLE, -1,
						(struct sockaddr *) &addr,
						sizeof(addr))));

	struct event *e;
	assert((e = evsignal_new(eb, SIGINT, signal_cb, (void *) eb)));
	assert(event_add(e, NULL) >= 0);

	event_base_dispatch(eb);

	evconnlistener_free(lstnr);
	event_free(e);

	return 0;
}

/**
 * help() - Selft explanatory.
 */
static void help() {
	puts("Server side code for a simple text toggle.");
	puts("Open a terminal on local and connect the localhost @port : 8888");
	puts("Example:-\n\tnc localhost 8888");
	puts("\nNow type any string, the server will convert that string to "	\
		"toggle case and will print it to your terminal");
}

/**
 * init_log - self explanatory.
 * @fname: log file name
 */
static void init_log(const char *fname) {
	logfile = fopen(fname, "a");
	event_enable_debug_logging(EVENT_DBG_ALL);
	event_set_log_callback(wtolog);
}

/**
 * wtolog - write to log file.
 * @sev: severity(level of logging event)
 * @msg: message to be written.
 */
static void wtolog(int sev, const char *msg) {
	char *s;
	switch (sev) {
	case EVENT_LOG_DEBUG:
		s = "DBG";
		break;
	case EVENT_LOG_MSG:
		s = "MSG";
		break;
	case EVENT_LOG_WARN:
		s = "WRN";
		break;
	case EVENT_LOG_ERR:
		s = "ERR";
		break;
	default:
		s = "???";
		break;
	}

	fprintf(logfile, "[%s] %s\n", s, msg);
}

/**
 * lstnr_cb - callback to the listener.
 * @lstnr: connection listener that received the connection.
 * @fd: new socket itself.
 * @addr: address from which the connection is recieved.
 * @len: length of the address.
 * @eb: user supplied pointer that was passed
 * 	(event_base for my case, just convert it explicity from void *)
 */
static void lstnr_cb(struct evconnlistener *lstnr, evutil_socket_t fd,
		     struct sockaddr *addr, int socklen, void *eb) {
	struct bufferevent *bev;

	assert((bev =
		bufferevent_socket_new(eb, fd, BEV_OPT_CLOSE_ON_FREE)));

	puts("New Connection Established.");

	bufferevent_setcb(bev, __rcb, NULL, event_cb, NULL);
	bufferevent_enable(bev, EV_WRITE | EV_READ);

	bufferevent_write(bev, wmsg, strlen(wmsg));
}

/**
 * event_cb - bufferevent event/error callback.
 * @bev: bufferevent for which the error was triggered.
 * @events: error flags
 * @ctx: user given context
 */
static void event_cb(struct bufferevent *bev, short events, void *ctx) {
	if (events & BEV_EVENT_CONNECTED) {
		puts("Connected");
	} else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
		if (events & BEV_EVENT_ERROR) {
			printf("Got an error on connection %s\n",
			       strerror(errno));
		}
		puts("[MSG] Closing connection");
		bufferevent_free(bev);
	}
}

/**
 * signal_cb - event cb, called when the interrupt is generated from the
 * 	server side.
 * @sig: file descriptor
 * @events: flags
 * @user_data: user given arguments.
 */
static void signal_cb(evutil_socket_t sig, short events, void *user_data) {
	struct event_base *base = user_data;
	struct timeval delay = { 1, 0 };
	puts("\n[SIGINT] Interrupt caught : closing in 1 sec.");
	event_base_loopexit(base, &delay);
}

/**
 * __rcb - reading callback.
 * @bev: bufferevent on which the signal for reading is generated.
 * @ctx: user given context
 */
void __rcb(struct bufferevent *bev, void *ctx) {
	char buff[1024];
	size_t n;
	/**
	 * We can even parse the string to perform some command line interface,
	 * like creating some cmd line chatbox, but since the task is to write
	 * a simple code and also since not much time is left, I am writing a
	 * basic code to toogle text.
	 */
	while ((n = bufferevent_read(bev, buff, sizeof(buff))) > 0) {
		for (int i = 0; i < n; i++) {
			if ((buff[i] >= 'a' && buff[i] <= 'z')
			    || (buff[i] >= 'A' && buff[i] <= 'Z'))
				buff[i] ^= (1 << 5);
		}
		bufferevent_write(bev, buff, n);
	}
}
