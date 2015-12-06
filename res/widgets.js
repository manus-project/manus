

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
//type="radio" name="options" id="option1" autocomplete="off" checked
        for (var key in options) {
            var button = $('<option/>').attr({'type': 'radio', 'name': key, 'autocomplete': 'off'}).text(options[key]['text']);
            container.append($('<label/>').addClass('btn btn-primary').append(button));
            button.click(options[key]['callback']);
        }
        
        return container;
    },

    jointWidget: function(parent, id, parameters) {

        var value = 0;

        var status;
        var information = $('<div class="information">').append($('<span class="title">').text("Joint " + (id+1))).append($('<span class="type">').text("Type: " + parameters.type));

        var container = $('<div class="joint">').append(information);

        switch (parameters.type) {
        case "translation":
        case "rotation": {

            status = $('<div class="status">').append();

            var current =  $('<div class="current">').appendTo(status);
            var goal =  $('<div class="goal">').appendTo(status);

            status.click(function(e) {
                var relative = Math.min(1, Math.max(0, (e.pageX - status.offset().left) / status.width()));

                var absolute = (parameters.max - parameters.min) * relative + parameters.min;

                $.ajax('/api/arm/move?joint=' + id + '&speed=1&position=' + absolute);

            });

            container.append(status);

            break;
        }
        case "gripper": {

            status = $('<button type="button" class="btn">Grip</button>').click(function() {
                var goal = 0;
                if ($(this).text() == "Release") goal = 1;
                $.ajax('/api/arm/move?joint=' + id + '&speed=1&position=' + goal);

            });

            container.append(status);
            break;
        }

        }

        $(parent).append(container);

        return function(v, g) {
            
            var value = parseFloat(v);
            var value_goal = parseFloat(g);

            var relative_position = (value - parameters.min) / (parameters.max - parameters.min);
            var relative_goal = (value_goal - parameters.min) / (parameters.max - parameters.min);

            switch (parameters.type) {
            case "translation": {
                status.attr('title', value.toFixed(2) + "mm");
                current.css('left', relative_position * status.width() - current.width() / 2);
                goal.css('left', relative_goal * status.width() - goal.width() / 2);
                break;
            }
            case "rotation": {
                status.attr('title', ((value * 180) / Math.PI ).toFixed(2) + "\u00B0");
                current.css('left', relative_position * status.width() - current.width() / 2);
                goal.css('left', relative_goal * status.width() - goal.width() / 2);
                break;
            }
            case "gripper": {
                if (value > 0) {
                    status.removeClass("btn-success").addClass("btn-danger");
                    status.text("Grip");
                } else {
                    status.removeClass("btn-danger").addClass("btn-success");
                    status.text("Release");
                }
                break;
            }

            }

        };

   }

};

})(jQuery);
