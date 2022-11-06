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
						@click="fetch_task_list()"
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
				item-key="name + file"
				open-on-click
				style="width: 100%"
			>
				<template v-slot:prepend="{ item, open }">
					<v-icon v-if="item.type === 'folder'">
						{{ open ? "mdi-folder-open" : "mdi-folder" }}
					</v-icon>
					<v-icon v-else>mdi-file-document-outline</v-icon>
				</template>
				<template v-slot:label="{ item }">
					<div v-if="item.type === 'task'" @click.stop="go(item)">
						{{ item.name }}
					</div>
					<div v-else>{{ item.name }}</div>
				</template>
				<template v-slot:append="{ item }">
					<v-row
						v-if="
							item.type === 'task' &&
							item.status !== undefined &&
							item.status !== null
						"
					>
						<div :class="'status_dot is-' + item.status.status"></div>
						<div class="info_job">#{{ item.status.job_number }}</div>
						<div class="info_date">
							{{ displayStarted(item.status.start_date) }}
						</div>
						<div class="info_duration">
							{{
								displayDuration(item.status.start_date, item.status.end_date)
							}}
						</div>
					</v-row>
				</template>
			</v-treeview>
		</v-row>
	</v-col>
</template>

<script>
import { DateTime } from "luxon";
import { EventBus } from "@/event-bus";

export default {
	name: "TaskList",
	data() {
		return {
			task_list: [{ name: "/" }],
			initiallyOpen: ["/"],
			selected_node: [],
		};
	},
	methods: {
		displayStarted(start_date) {
			const start = DateTime.fromSeconds(start_date);
			const end = DateTime.now();
			let diff_minutes = end.diff(start, "minutes");
			let minutes = diff_minutes.as("minutes").toFixed();
			let diff_hours = end.diff(start, "hours");
			let hours = diff_hours.as("hours").toFixed();
			let diff_days = end.diff(start, "days");
			let days = diff_days.as("days").toFixed();
			if (days > 0) {
				return `${start
					.toLocal()
					.toFormat("yyyy-LL-dd HH:mm:ss ZZZZ")} (${days} days ago)`;
			} else if (hours > 0) {
				return `${start
					.toLocal()
					.toFormat("yyyy-LL-dd HH:mm:ss ZZZZ")} (${hours} hours ago)`;
			} else {
				return `${start
					.toLocal()
					.toFormat("yyyy-LL-dd HH:mm:ss ZZZZ")} (${minutes} minutes ago)`;
			}
		},
		displayDuration(start_date, end_date) {
			const start = DateTime.fromSeconds(start_date);
			const end =
				end_date !== undefined
					? DateTime.fromSeconds(end_date)
					: DateTime.now();

			let diff = end
				.diff(start, ["hours", "minutes", "seconds", "milliseconds"])
				.toObject();
			diff.milliseconds = 0;
			const str = `${String(diff.hours).padStart(2, "0")}:${String(
				diff.minutes
			).padStart(2, "0")}:${String(diff.seconds).padStart(2, "0")}`;
			return str;
		},
		go(item) {
			const encoded_task = encodeURIComponent(
				(item.parent === "/" ? "" : item.parent) + "/" + item.name
			);
			this.$router.push(`/task/${encoded_task}/view`);
		},
		async fetch_task_list() {
			const res = await fetch(`${this.make_api_url()}/tasks`);
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
		this.$store.state.current_view = "TaskList";
	},
	mounted() {
		this.fetch_task_list();
		EventBus.$on("job:start", this.fetch_task_list);
		EventBus.$on("job:stop", this.fetch_task_list);
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
</style>
