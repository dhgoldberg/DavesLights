//var api_url = 'http://localhost:8080/api/room/0';
var api_url = document.URL + 'api/room/0';

$(document).on("pageinit","#pageone",function(){
    $( "#modeDropdown" ).change(function() {
        var data = {"mode":$("#modeDropdown").val().toString()};
        $.post(api_url, JSON.stringify(data));
        if ($("#modeDropdown").val() == "4") {
            $("#pickerDiv").show();
        } else {
            $("#pickerDiv").hide();
        }
        });

    $("#brightness").on('slidestop', function( event ) {
        var data = {"pixel_brightness": $("#brightness").val().toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#strobe").on('slidestop', function( event ) {
        var period = $("#strobe").val() * -1;
        var data = {'strobe_delay':period.toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#rainbow").on('slidestop', function( event ) {
        var period = $("#rainbow").val() * -1;
        var data = {'rainbow_delay': period.toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#spread").on('slidestop', function( event ) {
        var period = $("#spread").val() * -1;
        var data = {'co_spread_delay': period.toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#mindb").on('slidestop', function( event ) {
        var data = {'min_db':$("#mindb").val().toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#maxdb").on('slidestop', function( event ) {
        var data = {'max_db':$("#maxdb").val().toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#bassfreq").on('slidestop', function( event ) {
        var data = {'bass_freq':$("#bassfreq").val().toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#midfreq").on('slidestop', function( event ) {
        var data = {'mid_freq':$("#midfreq").val().toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#treblefreq").on('slidestop', function( event ) {
        var data = {'treble_freq':$("#treblefreq").val().toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#colorchange").on('slidestop', function( event ) {
        var data = {'color_change_cutoff':$("#colorchange").val().toString()};
        $.post(api_url, JSON.stringify(data));
        });

    $("#colorpickerHolder").ColorPicker({flat: true,
        //onChange: function(hsb, hex, rgb){
        onSubmit: function(hsb, hex, rgb, el){
            console.log(rgb.r);
            console.log(rgb.g);
            console.log(rgb.b);
            $.post(api_url + '/rgb', JSON.stringify(rgb));
        }
    });
    $("#pickerDiv").hide();

    if ($("#connectionStatusData").data().value == 'True') {
        $("#connectionStatusTrue").show();
        $("#connectionStatusFalse").hide();

        // here should go setConnected();
    } else {
        // here should go setDisconnected();
    }
});
