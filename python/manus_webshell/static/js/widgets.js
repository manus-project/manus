

(function($) {

if (!$.manus) {
    $.manus = {};
}

if (!$.manus.widgets) {
    $.manus.widgets = {};
}

$.manus.widgets = {

    toggleWidget: function(options) {

        var container = $('<div/>').addClass('btn-group').data('toggle', 'buttons');
        for (var key in options) {
            var button = $('<option/>').attr({'type': 'radio', 'name': key, 'autocomplete': 'off'}).text(options[key]['text']);
            container.append($('<label/>').addClass('btn btn-primary').append(button));
            button.click(options[key]['callback']);
        }

        return container;
    },

    buttons: function(options) {

        var container = $('<div/>').addClass('btn-group');
        for (var key in options) {
            var button = $('<a/>').addClass('btn btn-primary').attr({'role': 'button', 'name': key, 'autocomplete': 'off'}).text(options[key]['text']);
            container.append(button);
            button.click(options[key]['callback']);
        }

        return container;
    },

    fancybutton: function(options) {

        if (options["icon"]) {
            var button = $('<a/>').addClass('btn btn-primary').attr({'role': 'button', 'autocomplete': 'off'});
            if (options["text"]) button.text(options['text']);
            button.prepend($('<i/>').addClass('glyphicon glyphicon-' + options["icon"]));
            button.click(options['callback']);
            if (options["tooltip"]) {
                button.attr({title: options['tooltip']});
                button.tooltip({delay: 300, placement: "bottom"});
            } 
            return button;
        }

        var button = $('<a/>').addClass('btn btn-primary').attr({'role': 'button', 'name': key, 'autocomplete': 'off'}).text(options['text']);
        button.click(options['callback']);

        return button;
    },

    jointWidget: function(parent, manipulator, id, name, parameters) {

        var value = 0;

        var status;
        var information = $('<div class="information">').append($('<span class="title">').text(name)).append($('<span class="type">').text("Type: " + parameters.type));

        var container = $('<div class="joint">').append(information);

        switch (parameters.type.toLowerCase()) {
        case "translation":
        case "rotation": {

            status = $('<div class="status">').append();

            var current =  $('<div class="current">').appendTo(status);
            var goal =  $('<div class="goal">').appendTo(status);
            var dragging = false;

            status.click(function(e) {
                var relative = Math.min(1, Math.max(0, (e.pageX - status.offset().left) / status.width()));
                var absolute = (parameters.max - parameters.min) * relative + parameters.min;
                PubSub.publish(manipulator + '.move_joint', {id: id, position: absolute, speed: 1});
            });

            status.mousedown(function(e) {
                dragging = e.buttons == 1;
            })
            .mousemove(function(e) {
                var relative = Math.min(1, Math.max(0, (e.pageX - status.offset().left) / status.width()));
                var absolute = (parameters.max - parameters.min) * relative + parameters.min;
                if (!dragging) {
                    PubSub.publish(manipulator + '.hover', {id: id, position: absolute});
                } else {
                    if (e.buttons != 1) { dragging = false; return; }
                    PubSub.publish(manipulator + '.move_joint', {id: id, position: absolute, speed: 1});
                }
             })
            .mouseup(function() {
                dragging = false;
            }).mouseout(function() {
                dragging = false;
                PubSub.publish(manipulator + '.hover', {});
            });

            container.append(status);

            break;
        }
        case "gripper": {

            status = $('<button type="button" class="btn">Grip</button>').click(function() {
                var goal = 1;
                if ($(this).text() == "Release") goal = 0;

                PubSub.publish(manipulator + '.move_joint', {id: id, position: goal, speed: 1});

            });

            container.append(status);
            break;
        }

        }

        $(parent).append(container);

        PubSub.subscribe(manipulator + ".update", function(msg, data) {

            var g = data.joints[id].goal;
            var v = data.joints[id].position;

            var value = parseFloat(v);
            var value_goal = parseFloat(g);

            var relative_position = (value - parameters.min) / (parameters.max - parameters.min);
            var relative_goal = (value_goal - parameters.min) / (parameters.max - parameters.min);

            if (relative_position < 0) relative_position = 0;
            if (relative_position > 1) relative_position = 1;
            if (relative_goal < 0) relative_goal = 0;
            if (relative_goal > 1) relative_goal = 1;

            switch (parameters.type.toLowerCase()) {
            case "translation": {
                status.attr('title', value.toFixed(2) + "mm");
                current.css('left', relative_position * status.width() - current.width() / 2);
                goal.css('left', relative_goal * status.width() - goal.width() / 2);
                break;
            }
            case "rotation": {
                status.attr('title', (value * 180 / Math.PI).toFixed(2) + "\u00B0");
                current.css('left', relative_position * status.width() - current.width() / 2);
                goal.css('left', relative_goal * status.width() - goal.width() / 2);
                break;
            }
            case "gripper": {
                if (value < 0.1) {
                    status.removeClass("btn-success").addClass("btn-danger");
                    status.text("Grip");
                } else {
                    status.removeClass("btn-danger").addClass("btn-success");
                    status.text("Release");
                }
                break;
            }

            }

        });

    }

};

})(jQuery);
