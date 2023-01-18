import Vue from 'vue'
import App from './App.vue'
import router from './router'
import store from './store'
import vuetify from './plugins/vuetify'

Vue.config.productionTip = false

Vue.mixin({
	methods: {
		make_ws_url: function () {
			const ws_prot = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
			return process.env.NODE_ENV === "production" ?
				`${ws_prot}//${window.location.hostname}:${window.location.port}/ws` :
				`${ws_prot}//localhost:18080/ws`;
		},
		make_api_url: function () {
			return process.env.NODE_ENV === "production" ?
				`${window.location.protocol}//${window.location.hostname}:${window.location.port}/api` :
				'http://localhost:18080/api';
		},
	},
})

new Vue({
	router,
	store,
	vuetify,
	render: h => h(App)
}).$mount('#app')

