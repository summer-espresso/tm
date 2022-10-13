import Vue from 'vue'
import Vuex from 'vuex'

Vue.use(Vuex)

export default new Vuex.Store({
	state: {
		ws: null,
		ws_is_starting: false,
		ws_is_running: false,
		current_view: null,
	},
	getters: {
	},
	mutations: {
	},
	actions: {
	},
	modules: {
	}
})
