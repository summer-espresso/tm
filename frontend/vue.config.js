const { defineConfig } = require('@vue/cli-service')
module.exports = defineConfig({
	productionSourceMap: false,
	publicPath: process.env.NODE_ENV === 'production'
		? '/web/'
		: '/web/',
	transpileDependencies: [
		'vuetify'
	]
})
