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
								><v-icon>mdi-refresh-auto</v-icon>
							</v-btn>
						</template>
						<span v-if="autoRefreshValue === 0">Auto-refresh : OFF</span>
						<span v-if="autoRefreshValue > 0">Auto-refresh : Every {{ autoRefreshValue }} seconds</span>
					</v-tooltip>
				</template>
				<v-list>
					<v-subheader>Auto-refresh</v-subheader>
					<v-list-item v-for="(autoRefresh, index) in autoRefreshList" :key="index">
						<v-list-item-content>
							<v-btn
								:outlined="autoRefresh.value !== autoRefreshValue"
								color="primary"
								dark
								@click="set_auto_refresh(autoRefresh.value)"
								>{{ autoRefresh.title }}
							</v-btn>
						</v-list-item-content>
					</v-list-item>
				</v-list>
			</v-menu>
		</v-row>
		<v-row>
			<v-divider></v-divider>
		</v-row>
		<v-row>
			<v-treeview
				v-model="selected_node"
				:open="initiallyOpen"
				:items="task_list"
				open-on-click
				style="width: 100%"
				@update:open="cache_treeview"
			>
				<template v-slot:prepend="{ item, open }">
					<v-icon v-if="item.type === 'folder'">
						{{ open ? "mdi-folder-open" : "mdi-folder" }}
					</v-icon>
					<v-icon v-else>mdi-file-document-outline</v-icon>
				</template>
				<template v-slot:label="{ item }">
					<div v-if="item.type === 'task'" @click.stop="go(item)">
						<div>{{ item.name }}</div>
						<div class="task_desc" v-if="item.conf !== null">{{ item.conf.description }}</div>
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
			isLoaded: false,
			timerHandle: null,
			autoRefreshValue: 0,
			autoRefreshList: [{
				title: "OFF",
				value: 0,
			}, {
				title: "Every 1 second",
				value: 1,
			}, {
				title: "Every 5 seconds",
				value: 5,
			}, {
				title: "Every 10 seconds",
				value: 10,
			}, {
				title: "Every 30 seconds",
				value: 30,
			}, {
				title: "Every 1 minute",
				value: 60,
			}, {
				title: "Every 5 minutes",
				value: 60 * 5,
			}, {
				title: "Every 10 minutes",
				value: 60 * 10,
			}, {
				title: "Every 30 minutes",
				value: 60 * 30,
			}, {
				title: "Every 60 minutes",
				value: 60 * 60,
			}],
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
		cache_treeview(folder_opened) {
			if (this.isLoaded) {
				window.localStorage.setItem("folder_opened", JSON.stringify(folder_opened))
			}
		},
		set_timer() {
			if (this.timerHandle !== null) {
				clearInterval(this.timerHandle)
			}
			if (this.autoRefreshValue > 0) {
				this.timerHandle = setInterval(() => {
					this.fetch_task_list()
				}, this.autoRefreshValue * 1000)
			}
		},
		set_auto_refresh(auto_refresh) {
			window.localStorage.setItem("auto_refresh", auto_refresh)
			this.autoRefreshValue = auto_refresh
			this.set_timer()
		},
		async fetch_task_list() {
			const res = await fetch(`${this.make_api_url()}/tasks`, {
				cache: "no-cache",
			});
			const json = await res.json();
			this.task_list = json !== null ? json : [];
			if (!this.isLoaded) {
				const folder_opened = window.localStorage.getItem("folder_opened")
				try {
					const json_folder_opened = JSON.parse(folder_opened)
					if (Array.isArray(json_folder_opened)) {
						this.initiallyOpen = json_folder_opened
					}
				} catch (e) {
					// Nothing
				}
				try {
					this.autoRefreshValue = parseInt(window.localStorage.getItem("auto_refresh"), 10)
				} catch (e) {
					// Nothing
				}
				this.set_timer()
				this.isLoaded = true
			}
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
.task_desc {
	color:darkgrey;
}
</style>
