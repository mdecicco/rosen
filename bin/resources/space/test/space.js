class TestSpace {
	constructor() {
		engine.log('construct');
		this.test = 5;
	}
	
	initialize () {
		engine.log('init', this.test);
		const box = this.get_element('Box001');
		if (box) engine.log(box.name, box.id);
	}
	
	deinitialize () {
		engine.log('deinit', this.test);
	}
	
	update (dt) {
		engine.log(`update ${dt}`, this.test);
	}
};
