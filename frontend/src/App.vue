<template>
	<v-app id="inspire">
		<v-navigation-drawer permanent hide-overlay v-model="drawer" app>
			<v-list-item>
				<v-list-item-content>
					<v-list-item-title class="text-h6"> Task Manager </v-list-item-title>
					<v-list-item-subtitle> 0.3.2 </v-list-item-subtitle>
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
		items: [{ title: "Tasks", icon: "mdi-play-speed", path: "/" }],
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
				const len = e.data.length;
				if (len >= 5) {
					const prefix = e.data.substr(0, 5);
					if (prefix === "data:") {
						switch (this.$store.state.current_view) {
							case "JobView":
								EventBus.$emit("job:data", e.data.substr(5, len - 5));
								break;
						}
					}
					if (prefix === "stop:") {
						EventBus.$emit("job:stop");
					}
					if (prefix === "strt:") {
						EventBus.$emit("job:start");
					}
					if (prefix === "titl:") {
						EventBus.$emit("app:title", e.data.substr(5, len - 5));
					}
				}
			};
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
