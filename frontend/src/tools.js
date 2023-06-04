export const tools = {
	get_task (payload) {
		if (payload !== undefined && payload !== null && payload.length > 1) {
			const sep = payload.indexOf('|')
			if (sep >= 0) {
				return {
					job_number: payload.substr(0, sep),
					task_path: payload.substr(sep + 1),
				}
			}
		}
		return {
			job_number: -1,
			task_path: '',
		}
	}
}
