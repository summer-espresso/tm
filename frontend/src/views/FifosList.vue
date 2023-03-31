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
						@click="fetch_fifo_list()"
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
					<div>
						<div>{{ item.name }}</div>
					</div>
				</template>
				<template v-slot:append="{ item }">
					<v-row>
						<div class="info_job">last : {{ item.last }}</div>
						<div class="info_job">next : {{ item.next }}</div>
					</v-row>
				</template>
			</v-treeview>
		</v-row>
	</v-col>
</template>

<script>
import { EventBus } from "@/event-bus";

export default {
	name: "FifoList",
	data() {
		return {
			task_list: [{ name: "/" }],
			initiallyOpen: ["/"],
			selected_node: [],
		};
	},
	methods: {
		async fetch_fifo_list() {
			const res = await fetch(`${this.make_api_url()}/fifo/list`, {
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
		this.$store.state.current_view = "FifoList";
	},
	mounted() {
		this.fetch_fifo_list();
		EventBus.$on("fifo:push", this.fetch_fifo_list);
		EventBus.$on("fifo:pull", this.fetch_fifo_list);
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
