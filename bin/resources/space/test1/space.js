class TestSpace {
	constructor() {
		engine.log('construct');
		this.time = 0;
		this.chair = null;
	}
	
	initialize () {
		engine.log('init');
		this.chair = this.get_element('chair364');
		this.chair.set_update_frequency(60.0);
	}
	
	deinitialize () {
		engine.log('deinit');
	}
	
	update (dt) {
		this.time += dt;
		const transform = Transform3D.rotationY(this.time * 10.0);
		this.chair.transform = transform;
	}
};
