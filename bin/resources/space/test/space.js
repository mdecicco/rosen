class TestSpace {
	constructor() {
		engine.log('construct');
		this.time = 0;
		this.box = null;
	}
	
	initialize () {
		engine.log('init');
		this.box = this.get_element('Box001');
		this.box.set_update_frequency(60.0);
	}
	
	deinitialize () {
		engine.log('deinit');
	}
	
	update (dt) {
		this.time += dt;
		const transform = Transform3D.rotationY(this.time * 10.0);
		this.box.transform = transform;
	}
};
