<template>
	<v-col>
		<v-row>
			<v-tooltip bottom>
				<template v-slot:activator="{ on, attrs }">
					<v-btn
						v-bind="attrs"
						v-on="on"
						outlined
						color="blue"
						@click.stop="goStateList()"
						><v-icon>mdi-arrow-left</v-icon></v-btn
					>
				</template>
				<span>Bask to state list</span>
			</v-tooltip>
			<v-menu
				v-bind:close-on-click="true"
				v-bind:close-on-content-click="false"
				offset-y
			>
				<template v-slot:activator="{ on: menu, attrs }">
					<v-tooltip bottom>
						<template v-slot:activator="{ on: tooltip }">
							<v-btn
								outlined
								color="primary"
								dark
								v-bind="attrs"
								v-on="{ ...tooltip, ...menu }"
								><v-icon>mdi-dots-vertical</v-icon>
							</v-btn>
						</template>
						<span>API links</span>
					</v-tooltip>
				</template>
				<v-list>
					<v-subheader>API links</v-subheader>
					<v-list-item v-for="(link, index) in links" :key="index">
						<v-list-item-content>
							<v-text-field
								:label="link.title"
								:value="link.link"
								outlined
							></v-text-field>
						</v-list-item-content>
					</v-list-item>
				</v-list>
			</v-menu>
			<v-snackbar color="green" dark v-model="snackbar" :timeout="timeout">
				{{ text }}
				<template v-slot:action="{ attrs }">
					<v-btn color="white" text v-bind="attrs" @click="snackbar = false">
						Close
					</v-btn>
				</template>
			</v-snackbar>
		</v-row>
		<v-row>
			<v-divider></v-divider>
		</v-row>
		<v-row>
			<div style="width: 100%">
				<v-col>
					<h3>State {{ $route.params.path }}</h3>
					<div class="console_output"><pre>{{ state_content }}</pre></div>
				</v-col>
			</div>
		</v-row>
	</v-col>
</template>

<script>
import { EventBus } from "@/event-bus";

export default {
	name: "StateView",
	data() {
		return {
			state_content: '',
			links: [],
			snackbar: false,
			text: "",
			timeout: 3000,
		};
	},
	methods: {
		goStateList() {
			this.$router.push("/state");
		},
		async fetch_state_jobs() {
			const state_path = encodeURIComponent(this.$route.params.path);
			const res = await fetch(
				`${this.make_api_url()}/state/read/${state_path}`,
				{
					cache: "no-cache",
				}
			);
			const json = await res.json();
			this.state_content = JSON.stringify(json, null, 2);
		},
		dispatch_job_event(state_path) {
			if (state_path === this.$route.params.path) {
				this.fetch_state_jobs();
			}
		},
	},
	activated() {
		if (this.$store.state.ws !== null && this.$store.state.ws_is_running) {
			this.$store.state.ws.send(
				JSON.stringify({
					type: "unsubscribe",
				})
			);
		}
		const state_path = encodeURIComponent(this.$route.params.path);
		const api_url = this.make_api_url();
		this.links = [
			{
				icon: "mdi-play",
				title: "Store state (POST)",
				link: `${api_url}/state/store/${state_path}`,
			},
			{
				icon: "mdi-stop",
				title: "Read state (GET)",
				link: `${api_url}/state/read/${state_path}`,
			},
			{
				icon: "mdi-check",
				title: "Delete state (GET)",
				link: `${api_url}/state/delete/${state_path}`,
			},
		];
		this.$store.state.current_view = "StateView";
		this.fetch_state_jobs();
	},
	mounted() {
		EventBus.$on("state:store", this.dispatch_job_event);
		EventBus.$on("state:delete", this.dispatch_job_event);
	},
};
</script>

<style scoped>
.console_output {
	background-color: #3c3c3c;
	color: white;
	padding: 20px;
	box-shadow: 0px 1px 1px rgba(0, 0, 0, 0.29);
	border-radius: 3px;
	overflow-x: auto;
}

.console_output pre {
	margin: 0;
	padding: 0;
	white-space: break-spaces;
}
</style>
