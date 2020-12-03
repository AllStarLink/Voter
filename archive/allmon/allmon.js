
// when DOM is ready
$(document).ready(function() {

    // Is user authenticated
    if ($.cookie('allmon_loggedin') == 'yes') {
        $('#loginlink').hide();
    } else {
        $('#connect_form').hide();
        $('#logoutlink').hide();
    }
    
    // Login dialog
    $("#login").dialog( {
        autoOpen: false,
        title: 'Manager Login',
        modal: true,
        buttons: { "Login": function() {
            var user = $('form input:text').val();
            var passwd = $('input:password').val();
            $(this).dialog("close"); 
            $('#test_area').load("login.php", { 'user' : user, 'passwd' : passwd }, function(response) {
                if (response.substr(0,5) != 'Sorry') {
                    $('#connect_form').show();
                    $('#logoutlink').show();
                    $('#loginlink').hide();
                    $.cookie('allmon_loggedin', 'yes', { expires: 7, path: '/' });
                }
            });
            $('#test_area').stop().css('opacity', 1).fadeIn(50).delay(1000).fadeOut(2000);
        } }
    });

    // Login dialog opener
    $("#loginlink").click(function() {
        $("#login").dialog('open');
        return false;
    });
    
    // Logout 
    $('#logoutlink').click(function(event) {
        $.cookie('allmon_loggedin', null, { path: '/' });
        $('#loginlink').show();
        $('#logoutlink').hide();
        $('#connect_form').hide();
        event.preventDefault();
    });

    // Ajax connect a link
    $('#connect').click(function() {
        var node = $('#node').val(); 
        var perm = $('input:checkbox:checked').val();
        var localnode = $('#localnode').val();
        $.ajax( { url:'connect.php', data: { 'node' : node, 'perm' : perm, 'localnode' : localnode }, type:'post', success: function(result) {
                $('#test_area').html(result);
                $('#test_area').stop().css('opacity', 1).fadeIn(50).delay(1000).fadeOut(2000);
            }
        });
    });

    // Ajax disconnect a link
    $(document).on('click', 'a.disconnect', function(event) {
        var arr = $(this).attr('href').split('#');
        r = confirm("Disconnect Node " + arr[1] + "?");
        if (r == true) {
            var getParm = 'node=' + arr[0] + '&disconnect=' + arr[1];
            $('#test_area').load('disconnect.php', getParm);
            $('#test_area').hide().stop().css('opacity', 1).fadeIn(50).delay(5000).fadeOut(2000);
        }
        event.preventDefault();
    });

});