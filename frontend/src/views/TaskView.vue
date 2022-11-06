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
						@click.stop="goTaskList()"
						><v-icon>mdi-arrow-left</v-icon></v-btn
					>
				</template>
				<span>Bask to task list</span>
			</v-tooltip>
			<v-tooltip bottom>
				<template v-slot:activator="{ on, attrs }">
					<v-btn v-bind="attrs" v-on="on" color="green" @click="execute_task()"
						><v-icon>mdi-play</v-icon></v-btn
					>
				</template>
				<span>Execute</span>
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
					<h3>Task {{ $route.params.path }}</h3>
					<div class="table">
						<div class="table-header">
							<div>Execution</div>
							<div>Started</div>
							<div>Duration</div>
							<div>Exit code</div>
						</div>
						<div
							class="table-row pointer"
							v-for="job of job_list"
							:key="job.job_number"
							@click.stop="goJobView(job.job_number)"
						>
							<div>
								#{{ job.job_number }}
								<div :class="'status_dot is-' + job.status"></div>
							</div>
							<div>{{ displayStarted(job.start_date) }}</div>
							<div>{{ displayDuration(job.start_date, job.end_date) }}</div>
							<div>{{ job.exit_code }}</div>
						</div>
					</div>
				</v-col>
			</div>
		</v-row>
	</v-col>
</template>

<script>
import { DateTime } from "luxon";
import { EventBus } from "@/event-bus";

export default {
	name: "TaskView",
	data() {
		return {
			job_list: [],
			links: [],
			snackbar: false,
			text: "",
			timeout: 3000,
		};
	},
	methods: {
		goTaskList() {
			this.$router.push("/");
		},
		goJobView(num) {
			const task_path = encodeURIComponent(this.$route.params.path);
			this.$router.push(`/task/${task_path}/job/${num}/view`);
		},
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
		async execute_task() {
			const task_path = encodeURIComponent(this.$route.params.path);
			const res = await fetch(
				`${this.make_api_url()}/task/by_path/${task_path}/execute`
			);
			const json = await res.json();
			if (json.status === "ok") {
				this.$router.push(`/task/${task_path}/job/${json.job}/view`);
			}
		},
		async fetch_task_jobs() {
			const task_path = encodeURIComponent(this.$route.params.path);
			const res = await fetch(
				`${this.make_api_url()}/task/by_path/${task_path}/jobs`
			);
			const json = await res.json();
			this.job_list = json;
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
		const task_path = encodeURIComponent(this.$route.params.path);
		const api_url = this.make_api_url();
		this.links = [
			{
				icon: "mdi-play",
				title: "Execute task",
				link: `${api_url}/task/by_path/${task_path}/execute`,
			},
			{
				icon: "mdi-stop",
				title: "Stop task",
				link: `${api_url}/task/by_path/${task_path}/stop`,
			},
			{
				icon: "mdi-check",
				title: "Status task",
				link: `${api_url}/task/by_path/${task_path}/status`,
			},
		];
		this.$store.state.current_view = "TaskView";
		this.fetch_task_jobs();
	},
	mounted() {
		EventBus.$on("job:start", this.fetch_task_jobs);
		EventBus.$on("job:stop", this.fetch_task_jobs);
	},
};
</script>

<style scoped>
.pointer {
	cursor: pointer;
}
.table {
	display: table;
	border-spacing: 0 5px;
	width: 100%;
}

.table-header,
.table-row {
	display: table-row;
}

.table-header {
	font-weight: 100;
}

.table-row {
	margin-bottom: 10px;
}

.table-header > div,
.table-row > div {
	display: table-cell;
	padding: 5px;
}

.table-header > div > *,
.table-row > div > * {
	vertical-align: middle;
}

.table-header > div {
	padding-bottom: 0;
}

.table-row > div {
	background-color: white;
	box-shadow: 0px 1px 0px rgba(0, 0, 0, 0.1);
}

.table-row > div:first-child {
	border-top-left-radius: 3px;
	border-bottom-left-radius: 3px;
}

.table-row > div:last-child {
	border-top-right-radius: 3px;
	border-bottom-right-radius: 3px;
}

.table .execution_date {
	opacity: 0.4;
}

.table .execution_date.is-ago {
	font-size: 0.8em;
}
</style>
