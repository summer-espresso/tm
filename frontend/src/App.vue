<template>
	<v-app id="inspire">
		<v-navigation-drawer permanent hide-overlay v-model="drawer" app>
			<v-list-item>
				<v-list-item-content>
					<v-list-item-title class="text-h6"> Task Manager </v-list-item-title>
					<v-list-item-subtitle> 0.5.0 </v-list-item-subtitle>
				</v-list-item-content>
			</v-list-item>

			<v-divider></v-divider>

			<v-list dense nav>
				<v-list-item
					v-for="item in items"
					:key="item.title"
					:to="item.path"
					link
				>
					<v-list-item-icon>
						<v-icon>{{ item.icon }}</v-icon>
					</v-list-item-icon>

					<v-list-item-content>
						<v-list-item-title>{{ item.title }}</v-list-item-title>
					</v-list-item-content>
				</v-list-item>
			</v-list>
		</v-navigation-drawer>

		<v-main>
			<keep-alive>
				<router-view />
			</keep-alive>
		</v-main>
	</v-app>
</template>

<script>
import { EventBus } from "@/event-bus";

export default {
	data: () => ({
		drawer: false,
		items: [{
			title: "Tasks", icon: "mdi-play-speed", path: "/"
		}, {
			title: "Fifos", icon: "mdi-ray-start-arrow", path: "/fifo"
		}, {
			title: "States", icon: "mdi-database", path: "/state"
		}],
		right: null,
	}),
	methods: {
		init_ws() {
			if (this.$store.state.ws_is_starting || this.$store.state.ws_is_running) {
				return;
			}
			this.$store.state.ws_is_starting = true;
			// console.log("init_ws ...");
			if (this.$store.state.ws !== null) {
				delete this.$store.state.ws;
			}

			this.$store.state.ws = new WebSocket(`${this.make_ws_url()}`);

			this.$store.state.ws.onopen = () => {
				// console.log("ws open");
				this.$store.state.ws_is_running = true;
				this.$store.state.ws_is_starting = false;
			};

			this.$store.state.ws.onerror = () => {
				// console.log("ws error");
				this.$store.state.ws_is_running = false;
				this.$store.state.ws_is_starting = false;
			};

			this.$store.state.ws.onclose = () => {
				// console.log("ws close");
				this.$store.state.ws_is_running = false;
				this.$store.state.ws_is_starting = false;
			};

			this.$store.state.ws.onmessage = (e) => {
				// console.log("ws onmessage", e);
				const sep = e.data.indexOf('|')
				if (sep < 0) {
					return;
				}

				const prefix = e.data.substr(0, sep);
				const payload = e.data.substr(sep + 1);

				switch (prefix)
				{
					case "job:data":
						if (this.$store.state.current_view !== "JobView") {
							return;
						}
						break;

					case "state:store":
					case "state:delete":
						if (this.$store.state.current_view !== "StateList" && this.$store.state.current_view !== "StateView") {
							return;
						}
						break;

					case "fifo:push":
					case "fifo:pull":
						if (this.$store.state.current_view !== "FifoList") {
							return;
						}
						break;
				}

				EventBus.$emit(prefix, payload);
			}
		},
		change_title(title) {
			document.title = title;
		},
	},
	mounted() {
		this.init_ws();
		window.setInterval(this.init_ws, 1000);
		EventBus.$on("app:title", this.change_title);
	},
};
</script>

<style>
.execution_status_finished {
	font-weight: bold;
	color: white;
	padding: 0.3em 0.5em;
	border-radius: 3px;
	font-size: 0.7em;
	vertical-align: middle;
}
.status_dot {
	display: inline-block;
	/* vertical-align: middle; */
	background-color: gray;
	width: 14px;
	height: 14px;
	border-radius: 50%;
	margin: auto 10px auto 0.15em;
}
/* https://flatuicolors.com/palette/nl */
.execution_status_finished.is-running,
.status_dot.is-running {
	background-color: #5758bb;
}
.execution_status_finished.is-finished,
.status_dot.is-finished {
	background-color: #009432;
}
.execution_status_finished.is-interrupted,
.status_dot.is-interrupted {
	background-color: #eeb517;
}
.execution_status_finished.is-failed,
.status_dot.is-failed {
	background-color: #ea2027;
}
@keyframes blink {
	0% {
		opacity: 1;
	}
	50% {
		opacity: 0.5;
	}
	100% {
		opacity: 1;
	}
}
.is-running {
	animation-name: blink;
	animation-duration: 2s;
	animation-iteration-count: infinite;
	animation-timing-function: linear;
}
</style>
