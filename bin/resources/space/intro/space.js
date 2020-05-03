class TestSpace {
	constructor() {
		engine.log('construct');
		this.time = 0.0;
		this.rosens = [];
		this.stage_rosen = null;
		this.continue_crowd_speaking = false;
		this.actions = [];
	}
	
	addAction (time, actionCb) {
		this.actions.push({
			happened: false,
			time,
			callback: actionCb
		});
	}
	
	initialize () {
		engine.log('init');
		
		this.continue_crowd_speaking = true;
		this.activate_camera('audience_view');
		
		const stage_rosen_spawn = this.get_poi('stage_rosen_spawn');
		const stage_rosen_target = this.get_poi('stage_rosen_target');
		if (stage_rosen_spawn && stage_rosen_target) {
			engine.log('Spawning stage rosen');
			this.stage_rosen = this.spawn_rosen('stage_rosen', stage_rosen_spawn, false);
		}
		
		const spot = this.get_light('stage_spot_light');
		spot.transform = spot.transform.rotateZ(23.6);
		spot.lighting.cone_inner_angle = 20.0;
		spot.lighting.cone_outer_angle = 50.0;
		
		this.elements.filter(e => e.name.includes('chair')).forEach((chair, idx) => {
			if (Math.random() < 0.35) {
				const transform = chair.transform;
				transform.translate(new vec3f(-0.5, 1.5, 0));
				this.rosens.push(this.spawn_rosen(`rosen_${idx}`, transform, false));
			}
		});
		
		const start_time = 10.0;
		this.addAction(start_time + 0.0, () => {
			this.activate_camera('stage_view_0', 0.0);
			this.activate_camera('stage_view_1', 45.0);
		});
		
		this.addAction(start_time + 5.0, () => {
			this.stage_rosen.speech.lod_falloff_dist = 60.0;
			this.stage_rosen.speech.volume = 5.0;
			this.stage_rosen.speech.pitch = 0.9;
			
			this.stage_rosen.grab_frame('A Ball', 2.0);
			this.stage_rosen.set_transition('transform', engine.Interpolation.Linear, 20.0);
			this.stage_rosen.set_transition('speech.volume', engine.Interpolation.Linear, 5.0);
			this.stage_rosen.transform = stage_rosen_target;
			
			this.lights.forEach(l => {
				if (l.name.includes('stage_light_')) {
					l.set_transition('lighting.constant_attenuation', engine.Interpolation.EaseInOutCubic, 10.0);
					l.set_transition('lighting.linear_attenuation', engine.Interpolation.EaseInOutCubic, 10.0);
					l.lighting.constant_attenuation = 1.34;
					l.lighting.linear_attenuation = 1.1;
				}
			})
			
			spot.set_transition('lighting.cone_inner_angle', engine.Interpolation.EaseInOutCubic, 5.0);
			spot.set_transition('lighting.cone_outer_angle', engine.Interpolation.EaseInOutCubic, 5.0);
			spot.set_transition('lighting.quadratic_attenuation', engine.Interpolation.EaseInOutCubic, 5.0);
			spot.set_transition('transform', engine.Interpolation.EaseInOutCubic, 5.0);
			spot.transform = spot.transform.rotatedZ(-23.6).rotatedX(-20.0);
			spot.lighting.cone_inner_angle = 2.0;
			spot.lighting.cone_outer_angle = 2.5;
			spot.lighting.quadratic_attenuation = 0.0003;
		});
		
		this.addAction(start_time + 10.0, () => {
			spot.set_transition('transform', engine.Interpolation.Linear, 15.0);
			spot.transform = spot.transform.rotatedX(20.0);
		});
		
		this.addAction(start_time + 25.0, () => {
			this.continue_crowd_speaking = false;
		});
		
		this.addAction(start_time + 28.0, () => {
			this.stage_rosen.speak('quiet please *stares* can you hear me *attempts to unlock*');
		});
		
		this.addAction(start_time + 32.5, () => {
			const microphone = this.get_element('microphone');
			const mic_stand_top = this.get_element('microphone_stand_top');
			microphone.set_transition('transform', engine.Interpolation.EaseInCubic, 0.325);
			mic_stand_top.set_transition('transform', engine.Interpolation.EaseInCubic, 0.325);
			
			let mic_transform = microphone.transform;
			const stand_transform = mic_stand_top.transform;
			mic_transform = stand_transform.inverse().mul(mic_transform);
			stand_transform.translate(new vec3f(0, -0.78, 0));
			mic_transform = stand_transform.mul(mic_transform);
			
			microphone.transform = mic_transform;
			mic_stand_top.transform = stand_transform;
		});
		
		this.addAction(start_time + 32.0, () => {
			this.rosens.forEach(rosen => rosen.cancel_speech());
		});
		
		this.addAction(start_time + 33.0, () => {
			this.stage_rosen.speech.volume = 20.0;
			const speech_duration = this.stage_rosen.speak('dialog__intro_sequence');
			
			this.addAction(this.time + speech_duration + 4.5, () => {
				this.lights.forEach(l => {
					l.set_transition('lighting.constant_attenuation', engine.Interpolation.EaseInOutCubic, 38.0);
					l.set_transition('lighting.linear_attenuation', engine.Interpolation.EaseInOutCubic, 38.0);
					l.set_transition('lighting.quadratic_attenuation', engine.Interpolation.EaseInOutCubic, 28.0);
					l.lighting.constant_attenuation = 2.0;
					l.lighting.linear_attenuation = 2.0;
					l.lighting.quadratic_attenuation = 2.0;
				});
				
				const curtain_1 = this.get_element('curtain_1');
				curtain_1.set_transition('transform', engine.Interpolation.EaseInCubic, 10.0);
				curtain_1.transform = curtain_1.transform.scaled(new vec3f(0.05, 1.0, 1.0));
				const curtain_2 = this.get_element('curtain_2');
				curtain_2.set_transition('transform', engine.Interpolation.EaseInCubic, 10.0);
				curtain_2.transform = curtain_2.transform.translated(new vec3f(22.0, 0.0, 0.0)).scaled(new vec3f(0.05, 1.0, 1.0));
				
				this.stage_rosen.set_transition('transform', engine.Interpolation.Linear, 10.0);
				this.stage_rosen.transform = this.stage_rosen.translated(new vec3f(0, 0, 10.0));
			});
		});
	}
	
	deinitialize () {
		engine.log('deinit');
	}
	
	update (dt) {
		if (dt > 1.0) return;
		this.time += dt;
		this.actions.forEach(action => {
			if (action.happened) return;
			if (this.time > action.time) {
				action.callback();
				action.happened = true;
			}
		});
		
		if (this.continue_crowd_speaking) {
			this.rosens.forEach(rosen => {
				if (!rosen.is_speaking() && Math.random() < 0.01) rosen.speak_nonsense(3);
			});
		}
	}
};
