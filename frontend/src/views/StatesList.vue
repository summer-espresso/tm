<template>
	<v-col>
		<v-row>
			<v-tooltip right>
				<template v-slot:activator="{ on, attrs }">
					<v-btn
						v-bind="attrs"
						v-on="on"
						outlined
						color="blue"
						@click="fetch_state_list()"
						><v-icon>mdi-refresh</v-icon></v-btn
					>
				</template>
				<span>Refresh</span>
			</v-tooltip>
		</v-row>
		<v-row>
			<v-divider></v-divider>
		</v-row>
		<v-row>
			<v-treeview
				v-model="selected_node"
				:open="initiallyOpen"
				:items="task_list"
				activatable
				open-on-click
				style="width: 100%"
			>
				<template>
					<v-icon>mdi-file-document-outline</v-icon>
				</template>
				<template v-slot:label="{ item }">
					<div @click.stop="go(item)">
						<div>{{ item.name }}</div>
					</div>
				</template>
				<template>
					<v-row>
						<div class="info_job">-</div>
					</v-row>
				</template>
			</v-treeview>
		</v-row>
	</v-col>
</template>

<script>
import { EventBus } from "@/event-bus";

export default {
	name: "StateList",
	data() {
		return {
			task_list: [{ name: "/" }],
			initiallyOpen: ["/"],
			selected_node: [],
		};
	},
	methods: {
		go(item) {
			const encoded_task = encodeURIComponent(item.name);
			this.$router.push(`/state/${encoded_task}/view`);
		},
		async fetch_state_list() {
			const res = await fetch(`${this.make_api_url()}/state/list`, {
				cache: "no-cache",
			});
			const json = await res.json();
			this.task_list = json !== null ? json : [];
		},
	},
	activated() {
		const data = {
			type: "unsubscribe",
		};
		if (this.$store.state.ws !== null && this.$store.state.ws_is_running) {
			this.$store.state.ws.send(JSON.stringify(data));
		}
		this.$store.state.current_view = "StateList";
	},
	mounted() {
		this.fetch_state_list();
		EventBus.$on("state:store", this.fetch_state_list);
		EventBus.$on("state:delete", this.fetch_state_list);
	},
};
</script>

<style scoped>
.info_job {
	margin-right: 10px;
	font-weight: 500;
}
.info_date {
	margin-right: 10px;
	color: gray;
}
.info_duration {
	margin-right: 10px;
	color: gray;
}
.task_desc {
	color:darkgrey;
}
</style>
