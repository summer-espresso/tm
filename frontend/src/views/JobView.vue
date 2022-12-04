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
						@click.stop="go_task()"
						><v-icon>mdi-arrow-left</v-icon></v-btn
					>
				</template>
				<span>Bask to task</span>
			</v-tooltip>
			<v-btn :disabled="!is_running" color="red" @click.stop="stop_task()"
				><v-icon>mdi-stop</v-icon></v-btn
			>
		</v-row>
		<v-row>
			<v-divider></v-divider>
		</v-row>
		<v-row>
			<div style="width: 100%">
				<h3>Task {{ $route.params.path }}</h3>
				<h3>
					Job {{ $route.params.job }}
					<span
						id="job_execution_finished"
						:class="'execution_status_finished is-' + status.status"
					>
						{{ capitalizeFirstLetter(status.status) }}
					</span>
				</h3>
				<v-col>
					<h3 class="section_title">Execution</h3>
					<div
						class="console_output"
						v-if="html_content !== ''"
						v-html="html_content"
					></div>
					<div
						class="console_output"
						v-if="html_content === ''"
						id="job_execution_output"
					></div>
					<div>
						<span
							id="job_execution_finished"
							:class="'execution_status_finished is-' + status.status"
						>
							{{ capitalizeFirstLetter(status.status) }}
						</span>
					</div>
				</v-col>
			</div>
		</v-row>
	</v-col>
</template>

<script>
import { EventBus } from "@/event-bus";

export default {
	name: "JobView",
	data() {
		return {
			job_execution: true,
			html_content: "",
			current_task: "",
			current_job: "",
			is_running: false,
			last_is_running: false,
			status: {
				status: "",
			},
			exiting: false,
			keep_body_on_bottom: false,
		};
	},
	methods: {
		listen_events() {
			window.addEventListener("beforeunload", () => {
				this.exiting = true;
			});
			document.onkeydown = (e) => {
				// If "End" key is pressed, means that the user wants to go to the
				// end of the document. If the scrolling in this scenario is animated
				// (like in Firefox), it won't get to the bottom if lines are being
				// written in the exact moment, so it will not get "hooked" to the
				// output stream.
				//
				// Forcing the scroll top to the bottom removes the animation, but
				// always gets hooked to the output stream.
				if (e.key === "End" && this.is_running) {
					e.preventDefault();
					document.documentElement.scrollTop = document.body.scrollHeight;
					this.keep_body_on_bottom = true;
				} else {
					this.keep_body_on_bottom = false;
				}
			};
		},
		capitalizeFirstLetter(string) {
			return string.charAt(0).toUpperCase() + string.slice(1);
		},
		async stop_task() {
			const encoded_task = encodeURIComponent(this.$route.params.path);
			await fetch(`${this.make_api_url()}/task/by_path/${encoded_task}/stop`, {
				cache: "no-cache",
			});
			this.fetch_status();
			this.is_running = false;
		},
		go_task() {
			const encoded_task = encodeURIComponent(this.$route.params.path);
			this.$router.push(`/task/${encoded_task}/view`);
		},
		async fetch_status() {
			const task_path = encodeURIComponent(this.current_task);
			if (task_path !== "" && this.current_job !== -1) {
				const res = await fetch(
					`${this.make_api_url()}/task/by_path/${task_path}/job/${
						this.current_job
					}/status_log`
				);
				const status = await res.json();
				this.status = status;
				if (status.status !== undefined) {
					if (status.status === "running") {
						this.is_running = true;
					} else {
						this.is_running = false;
					}
					if (status.html !== undefined && status.html !== "") {
						this.html_content = status.html;
					} else if (status.content !== undefined && status.content !== "") {
						//
					}
				} else {
					this.is_running = false;
				}
			} else {
				this.is_running = false;
			}
			return this.is_running;
		},
		async fetch_log() {
			const task_path = encodeURIComponent(this.$route.params.path);
			const res = await fetch(
				`${this.make_api_url()}/task/by_path/${task_path}/job/${
					this.$route.params.job
				}/log`
			);
			const json = await res.json();
			this.html_content = json.html;
		},
		async fetch_output_stream() {
			const task_path = this.$route.params.path;
			const data = {
				type: "subscribe",
				task: task_path,
			};
			if (this.$store.state.ws !== null && this.$store.state.ws_is_running) {
				this.$store.state.ws.send(JSON.stringify(data));
			}
		},
		job_stop(task_path) {
			console.log("JobView : job_stop", task_path, this.$route.params.path);
			if (task_path === this.$route.params.path) {
				this.is_running = false;
				this.fetch_status();
				this.$nextTick(() => {
					if (this.keep_body_on_bottom) {
						document.documentElement.scrollTop = document.body.scrollHeight;
					}
				});
			}
		},
		job_data(data) {
			// https://github.com/sirikon/workr/blob/master/src/web/assets/job_execution.js
			const output_el = document.getElementById("job_execution_output");
			if (output_el !== null) {
				const line_el_base = document.createElement("pre");
				const output_last_line_el = line_el_base.cloneNode();
				output_last_line_el.textContent += data;
				output_el.appendChild(output_last_line_el);
			}

			if (this.keep_body_on_bottom) {
				document.documentElement.scrollTop = document.body.scrollHeight;
			}
		},
		job_clear(task_path) {
			console.log("JobView : job_clear", task_path, this.$route.params.path);
			if (task_path === this.$route.params.path) {
				this.output = "";
				this.html_content = "";
			}
		},
	},
	mounted() {
		this.listen_events();
		EventBus.$on("job:stop", this.job_stop);
		EventBus.$on("job:data", this.job_data);
		EventBus.$on("job:start", this.job_clear);
	},
	async activated() {
		const output_el = document.getElementById("job_execution_output");
		if (output_el !== null) {
			output_el.innerHTML = "";
		}
		this.$store.state.current_view = "JobView";
		this.html_content = "";
		this.current_task = this.$route.params.path;
		this.current_job = this.$route.params.job;
		await this.fetch_status();
		if (this.is_running) {
			this.fetch_output_stream();
		} else {
			// this.fetch_log();
		}
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
