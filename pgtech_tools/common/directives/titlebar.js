angular.module('common.titlebar', ['ui.bootstrap'])

.constant('titlebarConfig', {
    minimizable: true,
    maximizable: true,
    closable:    true
})

.controller('TitlebarController', ['$scope', '$attrs', 'titlebarConfig', 
function($scope, $attrs, titlebarConfig) {
    $scope.minimizable = angular.isDefined($attrs.minimizable) ? 
        $scope.$eval($attrs.minimizable) : titlebarConfig.minimizable;

    $scope.maximizable = angular.isDefined($attrs.maximizable) ? 
        $scope.$eval($attrs.maximizable) : titlebarConfig.maximizable;

    $scope.closable = angular.isDefined($attrs.closable) ? 
        $scope.$eval($attrs.closable) : titlebarConfig.closable;

    var win  = require('nw.gui').Window.get();
    $scope.maximized = false;

    win.on('maximize', function(){
        $scope.$apply(function () {
            $scope.maximized = true;
        });
    });

    win.on('unmaximize', function(){
        $scope.$apply(function () {
            $scope.maximized = false;
        });
    });

    //Maximize app
    $scope.maximize = function(){
        win.maximize();
    };
    
    //Unmaximize app
    $scope.unmaximize = function(){
        win.unmaximize();
    };

    //Minimize app
    $scope.minimize = function(){
        win.minimize();
    };

    //Close App
    $scope.close = function(){
        require('nw.gui').App.closeAllWindows();
    };
}])

.directive('titlebar', function() {
    return {
        restrict: 'EA',
        replace: true,
        transclude: true,
        controller: 'TitlebarController',
        templateUrl: '../common/templates/titlebar.html'
    };
});