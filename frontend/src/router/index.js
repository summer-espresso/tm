import Vue from 'vue'
import VueRouter from 'vue-router'
import TasksList from '../views/TasksList.vue'

Vue.use(VueRouter)

const routes = [
	{
		path: '/',
		name: 'tasks',
		component: TasksList
	},
	{
		// :path must be url encoded
		path: '/task/:path/view',
		name: 'view_task',
		component: () => import(/* webpackChunkName: "settings" */ '../views/TaskView.vue')
	},
	{
		// :path must be url encoded
		path: '/task/:path/job/:job/view',
		name: 'view_job',
		component: () => import(/* webpackChunkName: "settings" */ '../views/JobView.vue')
	},
]

const router = new VueRouter({
	routes
})

export default router
