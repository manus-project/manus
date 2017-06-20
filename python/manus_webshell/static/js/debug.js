(function($) {

if (!$.manus) {
    $.manus = {};
}

if (!$.manus.debug) {
    $.manus.debug = {};
}

$.manus.debug = {

    mat2html : function(mat) {
        var dim;

        switch (mat.length) {
            case 4:
                dim = 2;
                break;
            case 9:
                dim = 3;
                break;
            case 16:
                dim = 4;
                break;
            default: return 'undefined';
        }

        str = '';

        for (var i = 0; i < mat.length; i++) {
            str += mat[i].toFixed(3);
            if (i % dim == dim-1) str += '\n'; else str += ',';
        }

        return '<pre>' + str + '</pre>';

    }


}

})(jQuery);
